/*
    This file is part of the clazy static checker.

    Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
    Author: Sérgio Martins <sergio.martins@kdab.com>

    Copyright (C) 2015 Sergio Martins <smartins@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "qstring-allocations.h"
#include "ClazyContext.h"
#include "Utils.h"
#include "clazy_stl.h"
#include "StringUtils.h"
#include "FixItUtils.h"
#include "FunctionUtils.h"
#include "QtUtils.h"
#include "HierarchyUtils.h"
#include "SourceCompatibilityHelpers.h"

#include <clang/AST/DeclCXX.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/Expr.h>
#include <clang/Basic/Diagnostic.h>
#include <clang/Lex/Lexer.h>
#include <clang/AST/Decl.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/StmtIterator.h>
#include <clang/Basic/LLVM.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Frontend/CompilerInstance.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/raw_ostream.h>

#include <assert.h>

namespace clang {
class LangOptions;
class ParentMap;
class SourceManager;
}  // namespace clang

/// WARNING
///
/// This code is a bit unreadable and unmaintanable due to the fact that there are more corner-cases than normal cases.
/// It will be rewritten in a new check, so don't bother.

using namespace clang;
using namespace std;

enum Fixit {
    FixitNone = 0,
    QLatin1StringAllocations = 0x1,
    FromLatin1_FromUtf8Allocations = 0x2,
    CharPtrAllocations = 0x4,
};

struct Latin1Expr {
    CXXConstructExpr *qlatin1ctorexpr;
    bool enableFixit;
    bool isValid() const { return qlatin1ctorexpr != nullptr; }
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
    static const vector<StringRef> methods = {"append", "compare", "endsWith", "startsWith", "insert",
                                              "lastIndexOf", "prepend", "replace", "contains", "indexOf" };

    if (!clazy::isOfClass(method, "QString"))
        return false;

    return (!lt || Utils::isAscii(lt)) && clazy::contains(methods, clazy::name(method));
}

// Returns the first occurrence of a QLatin1String(char*) CTOR call
Latin1Expr QStringAllocations::qlatin1CtorExpr(Stmt *stm, ConditionalOperator * &ternary)
{
    if (!stm)
        return {};

    auto constructExpr = dyn_cast<CXXConstructExpr>(stm);
    if (constructExpr) {
        CXXConstructorDecl *ctor = constructExpr->getConstructor();
        const int numArgs = ctor->getNumParams();
        if (clazy::isOfClass(ctor, "QLatin1String")) {

            if (Utils::containsStringLiteral(constructExpr, /*allowEmpty=*/ false, 2))
                return {constructExpr, /*enableFixits=*/ numArgs == 1};

            if (Utils::userDefinedLiteral(constructExpr, "QLatin1String", lo()))
                return {constructExpr, /*enableFixits=*/ false};
        }
    }

    if (!ternary)
        ternary = dyn_cast<ConditionalOperator>(stm);

    for (auto child : stm->children()) {
        auto expr = qlatin1CtorExpr(child, ternary);
        if (expr.isValid())
            return expr;
    }

    return {};
}

// Returns true if there's a literal in the hierarchy, but aborts if it's parented on CallExpr
// so, returns true for: QLatin1String("foo") but false for QLatin1String(indirection("foo"));
//
static bool containsStringLiteralNoCallExpr(Stmt *stmt)
{
    if (!stmt)
        return false;

    auto sl = dyn_cast<StringLiteral>(stmt);
    if (sl)
        return true;

    for (auto child : stmt->children()) {
        if (!child)
            continue;
        auto callExpr = dyn_cast<CallExpr>(child);
        if (!callExpr && containsStringLiteralNoCallExpr(child))
            return true;
    }

    return false;
}

// For QString::fromLatin1("foo") returns "foo"
static StringLiteral* stringLiteralForCall(Stmt *call)
{
    if (!call)
        return nullptr;

    vector<StringLiteral*> literals;
    clazy::getChilds(call, literals, 2);
    return literals.empty() ? nullptr : literals[0];
}

void QStringAllocations::VisitCtor(Stmt *stm)
{
    auto ctorExpr = dyn_cast<CXXConstructExpr>(stm);
    if (!ctorExpr)
        return;

    if (!Utils::containsStringLiteral(ctorExpr, /**allowEmpty=*/ true))
        return;

    CXXConstructorDecl *ctorDecl = ctorExpr->getConstructor();
#if LLVM_VERSION_MAJOR >= 10
    // With llvm 10, for some reason, the child CXXConstructExpr of QStringList foo = {"foo}; aren't visited :(.
    // Do it manually.
    if (clazy::isOfClass(ctorDecl, "QStringList")) {
        auto p = clazy::getFirstChildOfType2<CXXConstructExpr>(ctorExpr);
        while (p) {
            if (clazy::isOfClass(p, "QString")) {
                VisitCtor(p);
            }
            p = clazy::getFirstChildOfType2<CXXConstructExpr>(p);
        }
    } else {
        VisitCtor(ctorExpr);
    }
#else
    VisitCtor(ctorExpr);
#endif
}

void QStringAllocations::VisitCtor(CXXConstructExpr *ctorExpr)
{
    CXXConstructorDecl *ctorDecl = ctorExpr->getConstructor();
    if (!clazy::isOfClass(ctorDecl, "QString"))
        return;

    if (Utils::insideCTORCall(m_context->parentMap, ctorExpr, { "QRegExp", "QIcon" })) {
        // https://blogs.kde.org/2015/11/05/qregexp-qstringliteral-crash-exit
        return;
    }

    if (!isOptionSet("no-msvc-compat")) {
        InitListExpr *initializerList = clazy::getFirstParentOfType<InitListExpr>(m_context->parentMap, ctorExpr);
        if (initializerList != nullptr)
            return; // Nothing to do here, MSVC doesn't like it

        StringLiteral *lt = stringLiteralForCall(ctorExpr);
        if (lt && lt->getNumConcatenated() > 1) {
            return; // Nothing to do here, MSVC doesn't like it
        }
    }

    bool isQLatin1String = false;
    string paramType;
    if (clazy::hasCharPtrArgument(ctorDecl, 1)) {
        paramType = "const char*";
    } else if (ctorDecl->param_size() == 1 && clazy::hasArgumentOfType(ctorDecl, "QLatin1String", lo())) {
        paramType = "QLatin1String";
        isQLatin1String = true;
    } else {
        return;
    }

    string msg = string("QString(") + paramType + string(") being called");

    if (isQLatin1String) {
        ConditionalOperator *ternary = nullptr;
        Latin1Expr qlatin1expr = qlatin1CtorExpr(ctorExpr, ternary);
        if (!qlatin1expr.isValid()) {
            return;
        }

        auto qlatin1Ctor = qlatin1expr.qlatin1ctorexpr;


        if (clazy::getLocStart(qlatin1Ctor).isMacroID()) {
            auto macroName = Lexer::getImmediateMacroName(clazy::getLocStart(qlatin1Ctor), sm(), lo());
            if (macroName == "Q_GLOBAL_STATIC_WITH_ARGS") // bug #391807
                return;
        }

        vector<FixItHint> fixits;
        if (qlatin1expr.enableFixit) {
            if (!clazy::getLocStart(qlatin1Ctor).isMacroID()) {
                if (!ternary) {
                    fixits = fixItReplaceWordWithWord(qlatin1Ctor, "QStringLiteral", "QLatin1String");
                    bool shouldRemoveQString = clazy::getLocStart(qlatin1Ctor).getRawEncoding() != clazy::getLocStart(ctorExpr).getRawEncoding() && dyn_cast_or_null<CXXBindTemporaryExpr>(clazy::parent(m_context->parentMap, ctorExpr));
                    if (shouldRemoveQString) {
                        // This is the case of QString(QLatin1String("foo")), which we just fixed to be QString(QStringLiteral("foo)), so now remove QString
                        auto removalFixits = clazy::fixItRemoveToken(&m_astContext, ctorExpr, true);
                        if (removalFixits.empty())  {
                            queueManualFixitWarning(clazy::getLocStart(ctorExpr), "Internal error: invalid start or end location");
                        } else {
                            clazy::append(removalFixits, fixits);
                        }
                    }
                } else {
                    fixits = fixItReplaceWordWithWordInTernary(ternary);
                }
            } else {
                queueManualFixitWarning(clazy::getLocStart(qlatin1Ctor), "Can't use QStringLiteral in macro");
            }
        }

        maybeEmitWarning(clazy::getLocStart(ctorExpr), msg, fixits);
    } else {
        vector<FixItHint> fixits;
        if (clazy::hasChildren(ctorExpr)) {
            auto pointerDecay = dyn_cast<ImplicitCastExpr>(*(ctorExpr->child_begin()));
            if (clazy::hasChildren(pointerDecay)) {
                auto lt = dyn_cast<StringLiteral>(*pointerDecay->child_begin());
                if (lt) {
                    Stmt *grandParent = clazy::parent(m_context->parentMap, lt, 2);
                    Stmt *grandGrandParent = clazy::parent(m_context->parentMap, lt, 3);
                    Stmt *grandGrandGrandParent = clazy::parent(m_context->parentMap, lt, 4);
                    if (grandParent == ctorExpr && grandGrandParent && isa<CXXBindTemporaryExpr>(grandGrandParent) && grandGrandGrandParent && isa<CXXFunctionalCastExpr>(grandGrandGrandParent)) {
                        // This is the case of QString("foo"), replace QString

                        const bool literalIsEmpty = lt->getLength() == 0;
                        if (literalIsEmpty && clazy::getFirstParentOfType<MemberExpr>(m_context->parentMap, ctorExpr) == nullptr)
                            fixits = fixItReplaceWordWithWord(ctorExpr, "QLatin1String", "QString");
                        else if (!clazy::getLocStart(ctorExpr).isMacroID())
                            fixits = fixItReplaceWordWithWord(ctorExpr, "QStringLiteral", "QString");
                        else
                            queueManualFixitWarning(clazy::getLocStart(ctorExpr), "Can't use QStringLiteral in macro.");
                    } else {

                        auto parentMemberCallExpr = clazy::getFirstParentOfType<CXXMemberCallExpr>(m_context->parentMap, lt, /*maxDepth=*/ 6); // 6 seems like a nice max from the ASTs I've seen

                        string replacement = "QStringLiteral";
                        if (parentMemberCallExpr) {
                            FunctionDecl *fDecl = parentMemberCallExpr->getDirectCallee();
                            if (fDecl) {
                                auto method = dyn_cast<CXXMethodDecl>(fDecl);
                                if (method && betterTakeQLatin1String(method, lt)) {
                                    replacement = "QLatin1String";
                                }
                            }
                        }

                        fixits = fixItRawLiteral(lt, replacement);
                    }
                }
            }
        }

        maybeEmitWarning(clazy::getLocStart(ctorExpr), msg, fixits);
    }
}

vector<FixItHint> QStringAllocations::fixItReplaceWordWithWord(clang::Stmt *begin, const string &replacement, const string &replacee)
{
    StringLiteral *lt = stringLiteralForCall(begin);
    if (replacee == "QLatin1String") {
        if (lt && !Utils::isAscii(lt)) {
            maybeEmitWarning(clazy::getLocStart(lt), "Don't use QLatin1String with non-latin1 literals");
            return {};
        }
    }

    if (Utils::literalContainsEscapedBytes(lt, sm(), lo()))
        return {};

    vector<FixItHint> fixits;
    FixItHint fixit = clazy::fixItReplaceWordWithWord(&m_astContext, begin, replacement, replacee);
    if (fixit.isNull()) {
        queueManualFixitWarning(clazy::getLocStart(begin), "");
    } else {
        fixits.push_back(fixit);
    }

    return fixits;
}

vector<FixItHint> QStringAllocations::fixItReplaceWordWithWordInTernary(clang::ConditionalOperator *ternary)
{
    vector<CXXConstructExpr*> constructExprs;

    auto addConstructExpr = [&constructExprs] (Expr* expr) {
        if (auto functionalCast = dyn_cast<CXXFunctionalCastExpr>(expr)) {
            expr = functionalCast->getSubExpr();
        }

        if (auto constructExpr = dyn_cast<CXXConstructExpr>(expr))
            constructExprs.push_back(constructExpr);
    };

    addConstructExpr(ternary->getTrueExpr());
    addConstructExpr(ternary->getFalseExpr());

    if (constructExprs.size() != 2) {
        llvm::errs() << "Weird ternary operator with " << constructExprs.size()
                     << " constructExprs at " << clazy::getLocStart(ternary).printToString(sm()) << "\n";
        ternary->dump();
        assert(false);
        return {};
    }

    vector<FixItHint> fixits;
    fixits.reserve(2);
    for (CXXConstructExpr *constructExpr : constructExprs) {
        SourceLocation rangeStart = clazy::getLocStart(constructExpr);
        SourceLocation rangeEnd = Lexer::getLocForEndOfToken(rangeStart, -1, sm(), lo());
        fixits.push_back(FixItHint::CreateReplacement(SourceRange(rangeStart, rangeEnd), "QStringLiteral"));
    }

    return fixits;
}

// true for: QString::fromLatin1().arg()
// false for: QString::fromLatin1("")
// true for: QString s = QString::fromLatin1("foo")
// false for: s += QString::fromLatin1("foo"), etc.
static bool isQStringLiteralCandidate(Stmt *s, ParentMap *map, const LangOptions &lo,
                                      const SourceManager &sm, int currentCall = 0)
{
    if (!s)
        return false;

    MemberExpr *memberExpr = dyn_cast<MemberExpr>(s);
    if (memberExpr)
        return true;

    auto constructExpr = dyn_cast<CXXConstructExpr>(s);
    if (clazy::isOfClass(constructExpr, "QString"))
        return true;

    if (Utils::isAssignOperator(dyn_cast<CXXOperatorCallExpr>(s), "QString", "QLatin1String", lo))
        return true;

    if (Utils::isAssignOperator(dyn_cast<CXXOperatorCallExpr>(s), "QString", "QString", lo))
        return true;

    CallExpr *callExpr = dyn_cast<CallExpr>(s);
    StringLiteral *literal = stringLiteralForCall(callExpr);
    auto operatorCall = dyn_cast<CXXOperatorCallExpr>(s);
    if (operatorCall && clazy::returnTypeName(operatorCall, lo) != "QTestData") {
        // QTest::newRow will static_assert when using QLatin1String
        // Q_STATIC_ASSERT_X(QMetaTypeId2<T>::Defined, "Type is not registered, please use the Q_DECLARE_METATYPE macro to make it known to Qt's meta-object system");

        string className = clazy::classNameFor(operatorCall);
        if (className == "QString") {
            return false;
        } else if (className.empty() && clazy::hasArgumentOfType(operatorCall->getDirectCallee(), "QString", lo)) {
            return false;
        }
    }

    if (currentCall > 0 && callExpr) {
        auto fDecl = callExpr->getDirectCallee();
        if (fDecl && betterTakeQLatin1String(dyn_cast<CXXMethodDecl>(fDecl), literal))
            return false;

        return true;
    }

    if (currentCall == 0 || dyn_cast<ImplicitCastExpr>(s) || dyn_cast<CXXBindTemporaryExpr>(s) || dyn_cast<MaterializeTemporaryExpr>(s)) // skip this cruft
        return isQStringLiteralCandidate(clazy::parent(map, s), map, lo, sm, currentCall + 1);

    return false;
}

std::vector<FixItHint> QStringAllocations::fixItReplaceFromLatin1OrFromUtf8(CallExpr *callExpr, FromFunction fromFunction)
{
    vector<FixItHint> fixits;

    std::string replacement = isQStringLiteralCandidate(callExpr, m_context->parentMap, lo(), sm()) ? "QStringLiteral"
                                                                                                    : "QLatin1String";
    if (replacement == "QStringLiteral" && clazy::getLocStart(callExpr).isMacroID()) {
        queueManualFixitWarning(clazy::getLocStart(callExpr), "Can't use QStringLiteral in macro!");
        return {};
    }

    StringLiteral *literal = stringLiteralForCall(callExpr);
    if (literal) {
        if (Utils::literalContainsEscapedBytes(literal, sm(), lo()))
            return {};
        if (!Utils::isAscii(literal)) {
            // QString::fromLatin1() to QLatin1String() is fine
            // QString::fromUtf8() to QStringLiteral() is fine
            // all other combinations are not
            if (replacement == "QStringLiteral" && fromFunction == FromLatin1) {
                return {};
            } else if (replacement == "QLatin1String" && fromFunction == FromUtf8) {
                replacement = "QStringLiteral";
            }
        }

        auto classNameLoc = Lexer::getLocForEndOfToken(clazy::getLocStart(callExpr), 0, sm(), lo());
        auto scopeOperatorLoc = Lexer::getLocForEndOfToken(classNameLoc, 0, sm(), lo());
        auto methodNameLoc = Lexer::getLocForEndOfToken(scopeOperatorLoc, -1, sm(), lo());
        SourceRange range(clazy::getLocStart(callExpr), methodNameLoc);
        fixits.push_back(FixItHint::CreateReplacement(range, replacement));
    } else {
        queueManualFixitWarning(clazy::getLocStart(callExpr), "Internal error: literal is null");
    }

    return fixits;
}

std::vector<FixItHint> QStringAllocations::fixItRawLiteral(clang::StringLiteral *lt, const string &replacement)
{
    vector<FixItHint> fixits;

    SourceRange range = clazy::rangeForLiteral(&m_astContext, lt);
    if (range.isInvalid()) {
        if (lt) {
            queueManualFixitWarning(clazy::getLocStart(lt), "Internal error: Can't calculate source location");
        }
        return {};
    }

    SourceLocation start = clazy::getLocStart(lt);
    if (start.isMacroID()) {
        queueManualFixitWarning(start, "Can't use QStringLiteral in macro");
    } else {
        if (Utils::literalContainsEscapedBytes(lt, sm(), lo()))
            return {};

        string revisedReplacement = lt->getLength() == 0 ? "QLatin1String" : replacement; // QLatin1String("") is better than QStringLiteral("")
        if (revisedReplacement == "QStringLiteral" && clazy::getLocStart(lt).isMacroID()) {
            queueManualFixitWarning(clazy::getLocStart(lt), "Can't use QStringLiteral in macro...");
            return {};
        }

        clazy::insertParentMethodCall(revisedReplacement, range, /**by-ref*/ fixits);
    }

    return fixits;
}

void QStringAllocations::VisitOperatorCall(Stmt *stm)
{
    auto operatorCall = dyn_cast<CXXOperatorCallExpr>(stm);
    if (!operatorCall)
        return;

    if (clazy::returnTypeName(operatorCall, lo()) == "QTestData") {
        // QTest::newRow will static_assert when using QLatin1String
        // Q_STATIC_ASSERT_X(QMetaTypeId2<T>::Defined, "Type is not registered, please use the Q_DECLARE_METATYPE macro to make it known to Qt's meta-object system");
        return;
    }

    std::vector<StringLiteral*> stringLiterals;
    clazy::getChilds<StringLiteral>(operatorCall, stringLiterals);

    //  We're only after string literals, str.contains(some_method_returning_const_char_is_fine())
    if (stringLiterals.empty())
        return;

    FunctionDecl *funcDecl = operatorCall->getDirectCallee();
    if (!funcDecl)
        return;

    auto methodDecl = dyn_cast<CXXMethodDecl>(funcDecl);
    if (!clazy::isOfClass(methodDecl, "QString"))
        return;

    if (!clazy::hasCharPtrArgument(methodDecl))
        return;

    vector<FixItHint> fixits;

    vector<StringLiteral*> literals;
    clazy::getChilds<StringLiteral>(stm, literals, 2);

    if (!isOptionSet("no-msvc-compat") && !literals.empty()) {
        if (literals[0]->getNumConcatenated() > 1) {
            return; // Nothing to do here, MSVC doesn't like it
        }
    }

    if (literals.empty()) {
        queueManualFixitWarning(clazy::getLocStart(stm), "Couldn't find literal");
    } else {
        const string replacement = Utils::isAscii(literals[0]) ? "QLatin1String" : "QStringLiteral";
        fixits = fixItRawLiteral(literals[0], replacement);
    }

    string msg = string("QString(const char*) being called");
    maybeEmitWarning(clazy::getLocStart(stm), msg, fixits);
}

void QStringAllocations::VisitFromLatin1OrUtf8(Stmt *stmt)
{
    auto callExpr = dyn_cast<CallExpr>(stmt);
    if (!callExpr)
        return;

    FunctionDecl *functionDecl = callExpr->getDirectCallee();
    if (!clazy::functionIsOneOf(functionDecl, {"fromLatin1", "fromUtf8"}))
        return;

    auto methodDecl = dyn_cast<CXXMethodDecl>(functionDecl);
    if (!clazy::isOfClass(methodDecl, "QString"))
        return;

    if (!Utils::callHasDefaultArguments(callExpr) || !clazy::hasCharPtrArgument(functionDecl, 2)) // QString::fromLatin1("foo", 1) is ok
        return;

    if (!containsStringLiteralNoCallExpr(callExpr))
        return;

    if (!isOptionSet("no-msvc-compat")) {
        StringLiteral *lt = stringLiteralForCall(callExpr);
        if (lt && lt->getNumConcatenated() > 1) {
            return; // Nothing to do here, MSVC doesn't like it
        }
    }

    vector<ConditionalOperator*> ternaries;
    clazy::getChilds(callExpr, ternaries, 2);
    if (!ternaries.empty()) {
        auto ternary = ternaries[0];
        if (Utils::ternaryOperatorIsOfStringLiteral(ternary)) {
            maybeEmitWarning(clazy::getLocStart(stmt), string("QString::fromLatin1() being passed a literal"));
        }

        return;
    }

    const FromFunction fromFunction = clazy::name(functionDecl) == "fromLatin1" ? FromLatin1 : FromUtf8;
    const std::vector<FixItHint> fixits = fixItReplaceFromLatin1OrFromUtf8(callExpr, fromFunction);

    if (clazy::name(functionDecl) == "fromLatin1") {
        maybeEmitWarning(clazy::getLocStart(stmt), string("QString::fromLatin1() being passed a literal"), fixits);
    } else {
        maybeEmitWarning(clazy::getLocStart(stmt), string("QString::fromUtf8() being passed a literal"), fixits);
    }
}

void QStringAllocations::VisitAssignOperatorQLatin1String(Stmt *stmt)
{
    auto callExpr = dyn_cast<CXXOperatorCallExpr>(stmt);
    if (!Utils::isAssignOperator(callExpr, "QString", "QLatin1String", lo()))
        return;

    if (!containsStringLiteralNoCallExpr(stmt))
        return;

    ConditionalOperator *ternary = nullptr;
    Stmt *begin = qlatin1CtorExpr(stmt, ternary).qlatin1ctorexpr;

    if (!begin)
        return;

    const vector<FixItHint> fixits = ternary == nullptr ? fixItReplaceWordWithWord(begin, "QStringLiteral", "QLatin1String")
                                                        : fixItReplaceWordWithWordInTernary(ternary);


    maybeEmitWarning(clazy::getLocStart(stmt), string("QString::operator=(QLatin1String(\"literal\")"), fixits);
}

void QStringAllocations::maybeEmitWarning(SourceLocation loc, string error, std::vector<FixItHint> fixits)
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

    emitWarning(loc, error, fixits);
}
