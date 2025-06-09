/*
    SPDX-FileCopyrightText: 2015 Klarälvdalens Datakonsult AB a KDAB Group company info@kdab.com
    SPDX-FileContributor: Sérgio Martins <sergio.martins@kdab.com>

    SPDX-FileCopyrightText: 2015 Sergio Martins <smartins@kde.org>
    SPDX-FileCopyrightText: 2024 Alexander Lohnau <alexander.lohnau@gmx.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "qstring-allocations.h"
#include "ClazyContext.h"
#include "FixItUtils.h"
#include "HierarchyUtils.h"
#include "QtUtils.h"
#include "StringUtils.h"
#include "Utils.h"
#include "clazy_stl.h"

#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/StmtIterator.h>
#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/LLVM.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Lex/Lexer.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/raw_ostream.h>

#include <assert.h>

#include <utility>

using namespace clang;

inline bool hasCharPtrArgument(clang::FunctionDecl *func, int expected_arguments = -1)
{
    if (expected_arguments != -1 && (int)func->param_size() != expected_arguments) {
        return false;
    }

    for (auto *param : Utils::functionParameters(func)) {
        if (clazy::startsWith(param->getType().getAsString(), "const char *")) { // On Qt6.8, an "&" is at the end
            return true;
        }
    }

    return false;
}

enum Fixit {
    FixitNone = 0,
    QLatin1StringAllocations = 0x1,
    FromLatin1_FromUtf8Allocations = 0x2,
    CharPtrAllocations = 0x4,
};

struct Latin1Expr {
    CXXConstructExpr *qlatin1ctorexpr;
    bool enableFixit;
    bool isValid() const
    {
        return qlatin1ctorexpr != nullptr;
    }
};

QStringAllocations::QStringAllocations(const std::string &name, ClazyContext *context)
    : CheckBase(name, context, Option_CanIgnoreIncludes)
{
}

void QStringAllocations::VisitStmt(clang::Stmt *stm)
{
    if (m_context->isQtDeveloper() && clazy::isBootstrapping(m_context->ci.getPreprocessorOpts())) {
        // During bootstrap many QString::fromLatin1() are used instead of tr(), which causes
        // much noise
        return;
    }

    VisitCtor(stm);
    VisitOperatorCall(stm);
    VisitFromLatin1OrUtf8(stm);
    VisitAssignOperatorQLatin1String(stm);
}

static bool betterTakeQLatin1String(CXXMethodDecl *method, StringLiteral *lt)
{
    static const std::vector<StringRef> methods = {
        "append",
        "compare",
        "endsWith",
        "startsWith",
        "insert",
        "lastIndexOf",
        "prepend",
        "replace",
        "contains",
        "indexOf",
    };

    if (!clazy::isOfClass(method, "QString")) {
        return false;
    }

    return (!lt || Utils::isAscii(lt)) && clazy::contains(methods, clazy::name(method));
}

// Returns the first occurrence of a QLatin1String(char*) CTOR call
Latin1Expr QStringAllocations::qlatin1CtorExpr(Stmt *stm, ConditionalOperator *&ternary)
{
    if (!stm) {
        return {};
    }

    auto *constructExpr = dyn_cast<CXXConstructExpr>(stm);
    if (constructExpr) {
        CXXConstructorDecl *ctor = constructExpr->getConstructor();
        const int numArgs = ctor->getNumParams();
        if (clazy::isOfClass(ctor, "QLatin1String")) {
            if (Utils::containsStringLiteral(constructExpr, /*allowEmpty=*/false, 2)) {
                return {constructExpr, /*enableFixits=*/numArgs == 1};
            }

            if (Utils::userDefinedLiteral(constructExpr, "QLatin1String", lo())) {
                return {constructExpr, /*enableFixits=*/false};
            }
        }
    }

    // C++17 elides the QLatin1String constructor
    if (Utils::userDefinedLiteral(stm, "QLatin1String", lo())) {
        return {constructExpr, /*enableFixits=*/false};
    }

    if (!ternary) {
        ternary = dyn_cast<ConditionalOperator>(stm);
    }

    for (auto *child : stm->children()) {
        auto expr = qlatin1CtorExpr(child, ternary);
        if (expr.isValid()) {
            return expr;
        }
    }

    return {};
}

// Returns true if there's a literal in the hierarchy, but aborts if it's parented on CallExpr
// so, returns true for: QLatin1String("foo") but false for QLatin1String(indirection("foo"));
//
static bool containsStringLiteralNoCallExpr(Stmt *stmt)
{
    if (!stmt) {
        return false;
    }

    auto *sl = dyn_cast<StringLiteral>(stmt);
    if (sl) {
        return true;
    }

    for (auto *child : stmt->children()) {
        if (!child) {
            continue;
        }
        auto *callExpr = dyn_cast<CallExpr>(child);
        if (!callExpr && containsStringLiteralNoCallExpr(child)) {
            return true;
        }
    }

    return false;
}

// For QString::fromLatin1("foo") returns "foo"
static StringLiteral *stringLiteralForCall(Stmt *call)
{
    if (!call) {
        return nullptr;
    }

    std::vector<StringLiteral *> literals;
    clazy::getChilds(call, literals, 3);
    return literals.empty() ? nullptr : literals[0];
}

void QStringAllocations::VisitCtor(Stmt *stm)
{
    auto *ctorExpr = dyn_cast<CXXConstructExpr>(stm);
    if (!ctorExpr) {
        return;
    }

    if (!Utils::containsStringLiteral(ctorExpr, /**allowEmpty=*/true)) {
        return;
    }

    // With llvm 10, for some reason, the child CXXConstructExpr of QStringList foo = {"foo}; aren't visited :(.
    // Do it manually.
    if (clazy::isOfClass(ctorExpr->getConstructor(), "QStringList")
        || ctorExpr->getConstructor()->getQualifiedNameAsString() == "QList<QString>::QList") { // In Qt6, QStringList is an alias
        auto *p = clazy::getFirstChildOfType2<CXXConstructExpr>(ctorExpr);
        while (p) {
            if (clazy::isOfClass(p, "QString")) {
                VisitCtor(p);
            }
            p = clazy::getFirstChildOfType2<CXXConstructExpr>(p);
        }
    } else {
        VisitCtor(ctorExpr);
    }
}

void QStringAllocations::VisitCtor(CXXConstructExpr *ctorExpr)
{
    CXXConstructorDecl *ctorDecl = ctorExpr->getConstructor();
    if (!clazy::isOfClass(ctorDecl, "QString")) {
        return;
    }

    if (Utils::insideCTORCall(m_context->parentMap, ctorExpr, {"QRegExp", "QIcon"})) {
        // https://blogs.kde.org/2015/11/05/qregexp-qstringliteral-crash-exit
        return;
    }

    if (!isOptionSet("no-msvc-compat")) {
        auto *initializerList = clazy::getFirstParentOfType<InitListExpr>(m_context->parentMap, ctorExpr);
        if (initializerList != nullptr) {
            return; // Nothing to do here, MSVC doesn't like it
        }

        StringLiteral *lt = stringLiteralForCall(ctorExpr);
        if (lt && lt->getNumConcatenated() > 1) {
            return; // Nothing to do here, MSVC doesn't like it
        }
    }

    bool isQLatin1String = false;
    std::string paramType;
    if (hasCharPtrArgument(ctorDecl, 1)) {
        paramType = "const char*";
    } else if (ctorDecl->param_size() == 1
               && (clazy::hasArgumentOfType(ctorDecl, "QLatin1String", lo()) || clazy::hasArgumentOfType(ctorDecl, "QLatin1StringView", lo()))) {
        paramType = "QLatin1String";
        isQLatin1String = true;
    } else {
        return;
    }

    std::string msg = std::string("QString(") + paramType + std::string(") being called");

    if (isQLatin1String) {
        ConditionalOperator *ternary = nullptr;
        Latin1Expr qlatin1expr = qlatin1CtorExpr(ctorExpr, ternary);
        if (!qlatin1expr.isValid()) {
            return;
        }

        auto *qlatin1Ctor = qlatin1expr.qlatin1ctorexpr;

        if (qlatin1Ctor->getBeginLoc().isMacroID()) {
            auto macroName = Lexer::getImmediateMacroName(qlatin1Ctor->getBeginLoc(), sm(), lo());
            if (macroName == "Q_GLOBAL_STATIC_WITH_ARGS") { // bug #391807
                return;
            }
        }

        std::vector<FixItHint> fixits;
        if (qlatin1expr.enableFixit) {
            if (!qlatin1Ctor->getBeginLoc().isMacroID()) {
                if (!ternary) {
                    fixits = fixItReplaceWordWithWord(qlatin1Ctor, "QStringLiteral", "QLatin1String");
                    bool shouldRemoveQString = qlatin1Ctor->getBeginLoc().getRawEncoding() != ctorExpr->getBeginLoc().getRawEncoding()
                        && dyn_cast_or_null<CXXBindTemporaryExpr>(clazy::parent(m_context->parentMap, ctorExpr));
                    if (shouldRemoveQString) {
                        // This is the case of QString(QLatin1String("foo")), which we just fixed to be QString(QStringLiteral("foo")), so now remove QString
                        auto removalFixits = clazy::fixItRemoveToken(&m_astContext, ctorExpr, true);
                        if (removalFixits.empty()) {
                            queueManualFixitWarning(ctorExpr->getBeginLoc(), "Internal error: invalid start or end location");
                        } else {
                            clazy::append(removalFixits, fixits);
                        }
                    }
                } else {
                    fixits = fixItReplaceWordWithWordInTernary(ternary);
                }
            } else {
                queueManualFixitWarning(qlatin1Ctor->getBeginLoc(), "Can't use QStringLiteral in macro");
            }
        }

        maybeEmitWarning(ctorExpr->getBeginLoc(), msg, fixits);
    } else {
        std::vector<FixItHint> fixits;
        if (clazy::hasChildren(ctorExpr)) {
            auto *pointerDecay = dyn_cast<ImplicitCastExpr>(*(ctorExpr->child_begin()));
            if (clazy::hasChildren(pointerDecay)) {
                auto *lt = dyn_cast<StringLiteral>(*pointerDecay->child_begin());
                if (lt) {
                    Stmt *grandParent = clazy::parent(m_context->parentMap, lt, 2);
                    Stmt *grandGrandParent = clazy::parent(m_context->parentMap, lt, 3);
                    Stmt *grandGrandGrandParent = clazy::parent(m_context->parentMap, lt, 4);
                    if (grandParent == ctorExpr && grandGrandParent && isa<CXXBindTemporaryExpr>(grandGrandParent) && grandGrandGrandParent
                        && isa<CXXFunctionalCastExpr>(grandGrandGrandParent)) {
                        // This is the case of QString("foo"), replace QString

                        const bool literalIsEmpty = lt->getLength() == 0;
                        if (literalIsEmpty && clazy::getFirstParentOfType<MemberExpr>(m_context->parentMap, ctorExpr) == nullptr) {
                            fixits = fixItReplaceWordWithWord(ctorExpr, "QLatin1String", "QString");
                        } else if (!ctorExpr->getBeginLoc().isMacroID()) {
                            fixits = fixItReplaceWordWithWord(ctorExpr, "QStringLiteral", "QString");
                        } else {
                            queueManualFixitWarning(ctorExpr->getBeginLoc(), "Can't use QStringLiteral in macro.");
                        }
                    } else {
                        auto *parentMemberCallExpr =
                            clazy::getFirstParentOfType<CXXMemberCallExpr>(m_context->parentMap,
                                                                           lt,
                                                                           /*maxDepth=*/6); // 6 seems like a nice max from the ASTs I've seen

                        std::string replacement = "QStringLiteral";
                        if (parentMemberCallExpr) {
                            FunctionDecl *fDecl = parentMemberCallExpr->getDirectCallee();
                            if (fDecl) {
                                auto *method = dyn_cast<CXXMethodDecl>(fDecl);
                                if (method && betterTakeQLatin1String(method, lt)) {
                                    replacement = "QLatin1String";
                                }
                            }
                        }

                        fixits = fixItRawLiteral(lt, replacement, nullptr);
                    }
                }
            }
        }

        maybeEmitWarning(ctorExpr->getBeginLoc(), msg, fixits);
    }
}

std::vector<FixItHint> QStringAllocations::fixItReplaceWordWithWord(clang::Stmt *begin, const std::string &replacement, const std::string &replacee)
{
    StringLiteral *lt = stringLiteralForCall(begin);
    if (replacee == "QLatin1String") {
        if (lt && !Utils::isAscii(lt)) {
            maybeEmitWarning(lt->getBeginLoc(), "Don't use QLatin1String with non-latin1 literals");
            return {};
        }
    }

    if (Utils::literalContainsEscapedBytes(lt, sm(), lo())) {
        return {};
    }

    std::vector<FixItHint> fixits;
    FixItHint fixit = clazy::fixItReplaceWordWithWord(&m_astContext, begin, replacement, replacee);
    if (fixit.isNull()) {
        queueManualFixitWarning(begin->getBeginLoc(), "");
    } else {
        fixits.push_back(fixit);
    }

    return fixits;
}

std::vector<FixItHint> QStringAllocations::fixItReplaceWordWithWordInTernary(clang::ConditionalOperator *ternary)
{
    std::vector<CXXConstructExpr *> constructExprs;

    auto addConstructExpr = [&constructExprs](Expr *expr) {
        if (auto *functionalCast = dyn_cast<CXXFunctionalCastExpr>(expr)) {
            expr = functionalCast->getSubExpr();
        }

        if (auto *constructExpr = dyn_cast<CXXConstructExpr>(expr)) {
            constructExprs.push_back(constructExpr);
        }
    };

    addConstructExpr(ternary->getTrueExpr());
    addConstructExpr(ternary->getFalseExpr());

    if (constructExprs.size() != 2) {
        llvm::errs() << "Weird ternary operator with " << constructExprs.size() << " constructExprs at " << ternary->getBeginLoc().printToString(sm()) << "\n";
        ternary->dump();
        assert(false);
        return {};
    }

    std::vector<FixItHint> fixits;
    fixits.reserve(2);
    for (CXXConstructExpr *constructExpr : constructExprs) {
        SourceLocation rangeStart = constructExpr->getBeginLoc();
        SourceLocation rangeEnd = Lexer::getLocForEndOfToken(rangeStart, -1, sm(), lo());
        fixits.push_back(FixItHint::CreateReplacement(SourceRange(rangeStart, rangeEnd), "QStringLiteral"));
    }

    return fixits;
}

// true for: QString::fromLatin1().arg()
// false for: QString::fromLatin1("")
// true for: QString s = QString::fromLatin1("foo")
// false for: s += QString::fromLatin1("foo"), etc.
static bool isQStringLiteralCandidate(Stmt *s, ParentMap *map, const LangOptions &lo, const SourceManager &sm, int currentCall = 0)
{
    if (!s) {
        return false;
    }

    auto *memberExpr = dyn_cast<MemberExpr>(s);
    if (memberExpr) {
        return true;
    }

    auto *constructExpr = dyn_cast<CXXConstructExpr>(s);
    if (clazy::isOfClass(constructExpr, "QString")) {
        return true;
    }

    if (Utils::isAssignOperator(dyn_cast<CXXOperatorCallExpr>(s), "QString", "QLatin1String", lo)) {
        return true;
    }

    if (Utils::isAssignOperator(dyn_cast<CXXOperatorCallExpr>(s), "QString", "QString", lo)) {
        return true;
    }

    auto *callExpr = dyn_cast<CallExpr>(s);
    StringLiteral *literal = stringLiteralForCall(callExpr);
    auto *operatorCall = dyn_cast<CXXOperatorCallExpr>(s);
    if (operatorCall && clazy::returnTypeName(operatorCall, lo) != "QTestData") {
        // QTest::newRow will static_assert when using QLatin1String
        // Q_STATIC_ASSERT_X(QMetaTypeId2<T>::Defined, "Type is not registered, please use the Q_DECLARE_METATYPE macro to make it known to Qt's meta-object
        // system");

        std::string className = clazy::classNameFor(operatorCall);
        if (className == "QString") {
            return false;
        }
        if (className.empty() && clazy::hasArgumentOfType(operatorCall->getDirectCallee(), "QString", lo)) {
            return false;
        }
    }

    // C++17 elides the QString constructor call in QString s = QString::fromLatin1("foo");
    if (currentCall > 0) {
        auto exprWithCleanups = dyn_cast<ExprWithCleanups>(s);
        if (exprWithCleanups) {
            if (auto bindTemp = dyn_cast<CXXBindTemporaryExpr>(exprWithCleanups->getSubExpr())) {
                if (dyn_cast<CallExpr>(bindTemp->getSubExpr()))
                    return true;
            }
        }
    }

    if (currentCall > 0 && callExpr) {
        auto *fDecl = callExpr->getDirectCallee();
        return !(fDecl && betterTakeQLatin1String(dyn_cast<CXXMethodDecl>(fDecl), literal));
    }

    if (currentCall == 0 || dyn_cast<ImplicitCastExpr>(s) || dyn_cast<CXXBindTemporaryExpr>(s)
        || dyn_cast<MaterializeTemporaryExpr>(s)) { // recurse over this cruft
        return isQStringLiteralCandidate(clazy::parent(map, s), map, lo, sm, currentCall + 1);
    }

    return false;
}

std::vector<FixItHint> QStringAllocations::fixItReplaceFromLatin1OrFromUtf8(CallExpr *callExpr, FromFunction fromFunction)
{
    std::vector<FixItHint> fixits;

    std::string replacement = isQStringLiteralCandidate(callExpr, m_context->parentMap, lo(), sm()) ? "QStringLiteral" : "QLatin1String";
    if (replacement == "QStringLiteral" && callExpr->getBeginLoc().isMacroID()) {
        queueManualFixitWarning(callExpr->getBeginLoc(), "Can't use QStringLiteral in macro!");
        return {};
    }

    StringLiteral *literal = stringLiteralForCall(callExpr);
    if (literal) {
        if (Utils::literalContainsEscapedBytes(literal, sm(), lo())) {
            return {};
        }
        if (!Utils::isAscii(literal)) {
            // QString::fromLatin1() to QLatin1String() is fine
            // QString::fromUtf8() to QStringLiteral() is fine
            // all other combinations are not
            if (replacement == "QStringLiteral" && fromFunction == FromLatin1) {
                return {};
            }
            if (replacement == "QLatin1String" && fromFunction == FromUtf8) {
                replacement = "QStringLiteral";
            }
        }

        auto classNameLoc = Lexer::getLocForEndOfToken(callExpr->getBeginLoc(), 0, sm(), lo());
        auto scopeOperatorLoc = Lexer::getLocForEndOfToken(classNameLoc, 0, sm(), lo());
        auto methodNameLoc = Lexer::getLocForEndOfToken(scopeOperatorLoc, -1, sm(), lo());
        SourceRange range(callExpr->getBeginLoc(), methodNameLoc);
        fixits.push_back(FixItHint::CreateReplacement(range, replacement));
    } else {
        queueManualFixitWarning(callExpr->getBeginLoc(), "Internal error: literal is null");
    }

    return fixits;
}

namespace
{
// Start at <loc> and go left as long as there's whitespace, stopping at <start> in the worst case
SourceLocation eatLeadingWhitespace(SourceLocation start, SourceLocation loc, const SourceManager &sm, const LangOptions &lo)
{
    const SourceRange range(start, loc);
    const CharSourceRange cr = Lexer::getAsCharRange(range, sm, lo);
    const StringRef str = Lexer::getSourceText(cr, sm, lo);
    const int initialPos = sm.getDecomposedLoc(loc).second - sm.getDecomposedLoc(start).second;
    int i = initialPos;
    while (--i >= 0) {
        if (!isHorizontalWhitespace(str[i]))
            return loc.getLocWithOffset(i - initialPos + 1);
    }
    return loc;
}
}

std::vector<FixItHint> QStringAllocations::fixItRawLiteral(StringLiteral *lt, const std::string &replacement, CXXOperatorCallExpr *operatorCall)
{
    std::vector<FixItHint> fixits;

    SourceRange range = clazy::rangeForLiteral(&m_astContext, lt);
    if (range.isInvalid()) {
        if (lt) {
            queueManualFixitWarning(lt->getBeginLoc(), "Internal error: Can't calculate source location");
        }
        return {};
    }

    SourceLocation start = lt->getBeginLoc();
    if (start.isMacroID()) {
        queueManualFixitWarning(start, "Can't use QStringLiteral in macro");
    } else {
        if (Utils::literalContainsEscapedBytes(lt, sm(), lo())) {
            return {};
        }

        // Turn str == "" into str.isEmpty()
        if (operatorCall && operatorCall->getOperator() == OO_EqualEqual && lt->getLength() == 0) {
            // For some reason this returns the same as getStartLoc
            // SourceLocation start = operatorCall->getArg(0)->getEndLoc();
            // So instead, we have to start from the "==" sign and eat whitespace to the left
            SourceLocation start = eatLeadingWhitespace(operatorCall->getBeginLoc(), operatorCall->getExprLoc(), sm(), lo());
            fixits.push_back(clazy::createReplacement({start, range.getEnd()}, ".isEmpty()"));
            return fixits;
        }

        // Turn str != "" into !str.isEmpty()
        if (operatorCall && operatorCall->getOperator() == OO_ExclaimEqual && lt->getLength() == 0) {
            // For some reason this returns the same as getStartLoc
            // SourceLocation start = operatorCall->getArg(0)->getEndLoc();
            // So instead, we have to start from the "==" sign and eat whitespace to the left
            SourceLocation start = eatLeadingWhitespace(operatorCall->getBeginLoc(), operatorCall->getExprLoc(), sm(), lo());
            fixits.push_back(clazy::createReplacement({start, range.getEnd()}, ".isEmpty()"));
            fixits.push_back(clazy::createInsertion(operatorCall->getBeginLoc(), "!"));
            return fixits;
        }

        std::string revisedReplacement = lt->getLength() == 0 ? "QLatin1String" : replacement; // QLatin1String("") is better than QStringLiteral("")
        if (revisedReplacement == "QStringLiteral" && lt->getBeginLoc().isMacroID()) {
            queueManualFixitWarning(lt->getBeginLoc(), "Can't use QStringLiteral in macro...");
            return {};
        }

        clazy::insertParentMethodCall(revisedReplacement, range, /**by-ref*/ fixits);
    }

    return fixits;
}

void QStringAllocations::VisitOperatorCall(Stmt *stm)
{
    auto *operatorCall = dyn_cast<CXXOperatorCallExpr>(stm);
    if (!operatorCall) {
        return;
    }

    if (clazy::returnTypeName(operatorCall, lo()) == "QTestData") {
        // QTest::newRow will static_assert when using QLatin1String
        // Q_STATIC_ASSERT_X(QMetaTypeId2<T>::Defined, "Type is not registered, please use the Q_DECLARE_METATYPE macro to make it known to Qt's meta-object
        // system");
        return;
    }

    std::vector<StringLiteral *> stringLiterals;
    clazy::getChilds<StringLiteral>(operatorCall, stringLiterals);

    //  We're only after string literals, str.contains(some_method_returning_const_char_is_fine())
    if (stringLiterals.empty()) {
        return;
    }

    FunctionDecl *funcDecl = operatorCall->getDirectCallee();
    if (!funcDecl) {
        return;
    }

    auto *methodDecl = dyn_cast<CXXMethodDecl>(funcDecl);
    if (methodDecl && !clazy::isOfClass(methodDecl, "QString")) {
        return;
    }

    // For Qt6.8, we have the operators in the QString class deprecated and have defined them using macros
    // Meaning we only have a function and /not/ a method
    if (!methodDecl && funcDecl->isThisDeclarationADefinition() && funcDecl->getBeginLoc().isValid()) {
        StringRef fileName = sm().getFilename(sm().getExpansionLoc(funcDecl->getBeginLoc()));
        if (!fileName.contains("qstring.h")) {
            return;
        }
    }

    if (!hasCharPtrArgument(funcDecl)) {
        return;
    }

    std::vector<FixItHint> fixits;

    std::vector<StringLiteral *> literals;
    clazy::getChilds<StringLiteral>(stm, literals, 3);

    if (!isOptionSet("no-msvc-compat") && !literals.empty()) {
        if (literals[0]->getNumConcatenated() > 1) {
            return; // Nothing to do here, MSVC doesn't like it
        }
    }

    if (literals.empty()) {
        queueManualFixitWarning(stm->getBeginLoc(), "Couldn't find literal");
    } else {
        const std::string replacement = Utils::isAscii(literals[0]) ? "QLatin1String" : "QStringLiteral";
        fixits = fixItRawLiteral(literals[0], replacement, operatorCall);
    }

    std::string msg("QString(const char*) being called");
    maybeEmitWarning(stm->getBeginLoc(), msg, fixits);
}

void QStringAllocations::VisitFromLatin1OrUtf8(Stmt *stmt)
{
    auto *callExpr = dyn_cast<CallExpr>(stmt);
    if (!callExpr) {
        return;
    }

    FunctionDecl *functionDecl = callExpr->getDirectCallee();
    if (!clazy::functionIsOneOf(functionDecl, {"fromLatin1", "fromUtf8"})) {
        return;
    }

    auto *methodDecl = dyn_cast<CXXMethodDecl>(functionDecl);
    if (!clazy::isOfClass(methodDecl, "QString")) {
        return;
    }

    bool isKnownLiteralOverload = false;
    for (auto e : Utils::functionParameters(functionDecl)) {
        if (e->getType().getAsString(lo()) == "QByteArrayView") {
            isKnownLiteralOverload = true;
        }
    }
    if (!isKnownLiteralOverload && (!Utils::callHasDefaultArguments(callExpr) || !hasCharPtrArgument(functionDecl, 2))) { // QString::fromLatin1("foo", 1) is ok
        return;
    }
    if (!containsStringLiteralNoCallExpr(callExpr)) {
        return;
    }

    if (!isOptionSet("no-msvc-compat")) {
        StringLiteral *lt = stringLiteralForCall(callExpr);
        if (lt && lt->getNumConcatenated() > 1) {
            return; // Nothing to do here, MSVC doesn't like it
        }
    }

    std::vector<ConditionalOperator *> ternaries;
    clazy::getChilds(callExpr, ternaries); // In Qt5 it is always 2 levels down, but in Qt6 more
    if (!ternaries.empty()) {
        auto *ternary = ternaries[0];
        if (Utils::ternaryOperatorIsOfStringLiteral(ternary)) {
            maybeEmitWarning(stmt->getBeginLoc(), std::string("QString::fromLatin1() being passed a literal"));
        }

        return;
    }

    const FromFunction fromFunction = clazy::name(functionDecl) == "fromLatin1" ? FromLatin1 : FromUtf8;
    const std::vector<FixItHint> fixits = fixItReplaceFromLatin1OrFromUtf8(callExpr, fromFunction);

    if (clazy::name(functionDecl) == "fromLatin1") {
        maybeEmitWarning(stmt->getBeginLoc(), std::string("QString::fromLatin1() being passed a literal"), fixits);
    } else {
        maybeEmitWarning(stmt->getBeginLoc(), std::string("QString::fromUtf8() being passed a literal"), fixits);
    }
}

void QStringAllocations::VisitAssignOperatorQLatin1String(Stmt *stmt)
{
    auto *callExpr = dyn_cast<CXXOperatorCallExpr>(stmt);
    if (!callExpr) {
        return;
    }
    if (!Utils::isAssignOperator(callExpr, "QString", "QLatin1String", lo()) && !Utils::isAssignOperator(callExpr, "QString", "QLatin1StringView", lo())) {
        return;
    }

    if (!containsStringLiteralNoCallExpr(stmt)) {
        return;
    }

    ConditionalOperator *ternary = nullptr;
    Stmt *begin = qlatin1CtorExpr(stmt, ternary).qlatin1ctorexpr;

    if (!begin) {
        return;
    }

    const std::vector<FixItHint> fixits =
        ternary == nullptr ? fixItReplaceWordWithWord(begin, "QStringLiteral", "QLatin1String") : fixItReplaceWordWithWordInTernary(ternary);

    maybeEmitWarning(stmt->getBeginLoc(), std::string("QString::operator=(QLatin1String(\"literal\")"), fixits);
}

void QStringAllocations::maybeEmitWarning(SourceLocation loc, std::string error, std::vector<FixItHint> fixits)
{
    if (clazy::isUIFile(loc, sm())) {
        // Don't bother warning for generated UI files.
        // We do the check here instead of at the beginning so users that don't use UI files don't have to pay the performance price.
        return;
    }

    if (m_context->isQtDeveloper() && Utils::filenameForLoc(loc, sm()) == "qstring.cpp") {
        // There's an error replacing an internal fromLatin1() because the replacement code doesn't expect to be working on QString itself
        // not worth to fix, it's only 1 case in qstring.cpp, and related to Qt 1.x compat
        fixits = {};
    }

    emitWarning(loc, std::move(error), fixits);
}
