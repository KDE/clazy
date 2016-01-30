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

#include "qstringuneededheapallocations.h"
#include "Utils.h"
#include "clazy_stl.h"
#include "StringUtils.h"
#include "FixItUtils.h"
#include "MethodSignatureUtils.h"
#include "checkmanager.h"

#include <clang/AST/DeclCXX.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/Expr.h>
#include <clang/Basic/Diagnostic.h>
#include <clang/Rewrite/Frontend/FixItRewriter.h>
#include <clang/Lex/Lexer.h>
#include <clang/AST/ParentMap.h>

#include <iostream>

using namespace clang;
using namespace std;

enum Fixit {
    FixitNone = 0,
    QLatin1StringAllocations = 0x1,
    FromLatin1_FromUtf8Allocations = 0x2,
    CharPtrAllocations = 0x4,
};

QStringUneededHeapAllocations::QStringUneededHeapAllocations(const std::string &name, const clang::CompilerInstance &ci)
    : CheckBase(name, ci)
{
}

void QStringUneededHeapAllocations::VisitStmt(clang::Stmt *stm)
{
    VisitCtor(stm);
    VisitOperatorCall(stm);
    VisitFromLatin1OrUtf8(stm);
    VisitAssignOperatorQLatin1String(stm);
}

static bool betterTakeQLatin1String(CXXMethodDecl *method, StringLiteral *lt)
{
    // indexOf() and contains() are slower, don't include it. They internally call qt_from_latin1() making them 30% slower than QStringLiteral
    static const vector<string> methods = {"append", "compare", "endsWith", "startsWith", "insert", "lastIndexOf", "prepend", "replace" };

    if (!isOfClass(method, "QString"))
        return false;

    return Utils::isAscii(lt) && clazy_std::contains(methods, method->getNameAsString());
}

// Returns the first occurrence of a QLatin1String(char*) CTOR call
static CXXConstructExpr *qlatin1CtorExpr(Stmt *stm, ConditionalOperator * &ternary)
{
    if (!stm)
        return nullptr;

    CXXConstructExpr *constructExpr = dyn_cast<CXXConstructExpr>(stm);
    if (constructExpr) {
        CXXConstructorDecl *ctor = constructExpr->getConstructor();
        if (isOfClass(ctor, "QLatin1String") && hasCharPtrArgument(ctor, 1)) {
            if (Utils::containsStringLiteral(constructExpr, /*allowEmpty=*/ false, 2))
                return constructExpr;
        }
    }

    if (!ternary)
        ternary = dyn_cast<ConditionalOperator>(stm);

    for (auto child : stm->children()) {
        auto expr = qlatin1CtorExpr(child, ternary);
        if (expr)
            return expr;
    }

    return nullptr;
}

// Returns true if there's a literal in the hierarchy, but aborts if it's parented on CallExpr
// so, returns true for: QLatin1String("foo") but false for QLatin1String(indirection("foo"));
//
static bool containsStringLiteralNoCallExpr(Stmt *stmt)
{
    if (!stmt)
        return false;

    StringLiteral *sl = dyn_cast<StringLiteral>(stmt);
    if (sl)
        return true;

    for (auto child : stmt->children()) {
        if (!child)
            continue;
        CallExpr *callExpr = dyn_cast<CallExpr>(child);
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
    HierarchyUtils::getChilds(call, literals, 2);
    return literals.empty() ? nullptr : literals[0];
}

void QStringUneededHeapAllocations::VisitCtor(Stmt *stm)
{
    CXXConstructExpr *ctorExpr = dyn_cast<CXXConstructExpr>(stm);
    if (!Utils::containsStringLiteral(ctorExpr, /**allowEmpty=*/ true))
        return;

    CXXConstructorDecl *ctorDecl = ctorExpr->getConstructor();
    if (!isOfClass(ctorDecl, "QString"))
        return;

    static const vector<string> blacklistedParentCtors = { "QRegExp" };
    if (Utils::insideCTORCall(m_parentMap, stm, blacklistedParentCtors)) {
        // https://blogs.kde.org/2015/11/05/qregexp-qstringliteral-crash-exit
        return;
    }

    if (!isOptionSet("no-msvc-compat")) {
        InitListExpr *initializerList = HierarchyUtils::getFirstParentOfType<InitListExpr>(m_parentMap, ctorExpr);
        if (initializerList != nullptr)
            return; // Nothing to do here, MSVC doesn't like it

        StringLiteral *lt = stringLiteralForCall(stm);
        if (lt && lt->getNumConcatenated() > 1) {
            return; // Nothing to do here, MSVC doesn't like it
        }
    }

    bool isQLatin1String = false;
    string paramType;
    if (hasCharPtrArgument(ctorDecl, 1)) {
        paramType = "const char*";
    } else if (hasArgumentOfType(ctorDecl, "class QLatin1String", 1)) {
        paramType = "QLatin1String";
        isQLatin1String = true;
    } else {
        return;
    }

    string msg = string("QString(") + paramType + string(") being called");

    if (isQLatin1String) {
        ConditionalOperator *ternary = nullptr;
        CXXConstructExpr *qlatin1Ctor = qlatin1CtorExpr(stm, ternary);
        if (!qlatin1Ctor) {
            return;
        }

        vector<FixItHint> fixits;
        if (isFixitEnabled(QLatin1StringAllocations)) {
            if (!qlatin1Ctor->getLocStart().isMacroID()) {
                if (ternary == nullptr) {
                    fixits = fixItReplaceWordWithWord(qlatin1Ctor, "QStringLiteral", "QLatin1String", QLatin1StringAllocations);
                    bool shouldRemoveQString = qlatin1Ctor->getLocStart().getRawEncoding() != stm->getLocStart().getRawEncoding() && dyn_cast_or_null<CXXBindTemporaryExpr>(HierarchyUtils::parent(m_parentMap, ctorExpr));
                    if (shouldRemoveQString) {
                        // This is the case of QString(QLatin1String("foo")), which we just fixed to be QString(QStringLiteral("foo)), so now remove QString
                        auto removalFixits = FixItUtils::fixItRemoveToken(ci(), ctorExpr, true);
                        if (removalFixits.empty())  {
                            queueManualFixitWarning(ctorExpr->getLocStart(), QLatin1StringAllocations, "Internal error: invalid start or end location");
                        } else {
                            clazy_std::copy(removalFixits, fixits);
                        }
                    }
                } else {
                    fixits = fixItReplaceWordWithWordInTernary(ternary);
                }
            } else {
                queueManualFixitWarning(qlatin1Ctor->getLocStart(), QLatin1StringAllocations, "Can't use QStringLiteral in macro");
            }
        }

        emitWarning(stm->getLocStart(), msg, fixits);
    } else {
        vector<FixItHint> fixits;
        if (ctorExpr->child_begin() != ctorExpr->child_end()) {
            auto pointerDecay = dyn_cast<ImplicitCastExpr>(*(ctorExpr->child_begin()));
            if (pointerDecay && pointerDecay->child_begin() != pointerDecay->child_end()) {
                StringLiteral *lt = dyn_cast<StringLiteral>(*pointerDecay->child_begin());
                if (lt && isFixitEnabled(CharPtrAllocations)) {
                    Stmt *grandParent = HierarchyUtils::parent(m_parentMap, lt, 2);
                    Stmt *grandGrandParent = HierarchyUtils::parent(m_parentMap, lt, 3);
                    Stmt *grandGrandGrandParent = HierarchyUtils::parent(m_parentMap, lt, 4);
                    if (grandParent == ctorExpr && grandGrandParent && isa<CXXBindTemporaryExpr>(grandGrandParent) && grandGrandGrandParent && isa<CXXFunctionalCastExpr>(grandGrandGrandParent)) {
                        // This is the case of QString("foo"), replace QString
                        //llvm::errs() << "case1\n";
                        const bool literalIsEmpty = lt->getLength() == 0;
                        if (literalIsEmpty)
                            fixits = fixItReplaceWordWithWord(ctorExpr, "QLatin1String", "QString", CharPtrAllocations);
                        else if (!ctorExpr->getLocStart().isMacroID())
                            fixits = fixItReplaceWordWithWord(ctorExpr, "QStringLiteral", "QString", CharPtrAllocations);
                        else
                            queueManualFixitWarning(ctorExpr->getLocStart(), CharPtrAllocations, "Can't use QStringLiteral in macro.");
                    } else {
                        //llvm::errs() << "case2\n";

                        auto parentMemberCallExpr = HierarchyUtils::getFirstParentOfType<CXXMemberCallExpr>(m_parentMap, lt, /*maxDepth=*/6); // 6 seems like a nice max from the ASTs I've seen

                        string replacement = "QStringLiteral";
                        if (parentMemberCallExpr) {
                            FunctionDecl *fDecl = parentMemberCallExpr->getDirectCallee();
                            if (fDecl) {
                                CXXMethodDecl *method = dyn_cast<CXXMethodDecl>(fDecl);
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

        emitWarning(stm->getLocStart(), msg, fixits);
    }
}

vector<FixItHint> QStringUneededHeapAllocations::fixItReplaceWordWithWord(clang::Stmt *begin, const string &replacement, const string &replacee, int fixitType)
{
    if (replacee == "QLatin1String") {
        StringLiteral *lt = stringLiteralForCall(begin);
        if (lt && !Utils::isAscii(lt)) {
            emitWarning(lt->getLocStart(), "Don't use QLatin1String with non-latin1 literals");
            return {};
        }
    }

    vector<FixItHint> fixits;
    FixItHint fixit = FixItUtils::fixItReplaceWordWithWord(ci(), begin, replacement, replacee);
    if (fixit.isNull()) {
        queueManualFixitWarning(begin->getLocStart(), fixitType);
    } else {
        fixits.push_back(fixit);
    }

    return fixits;
}

vector<FixItHint> QStringUneededHeapAllocations::fixItReplaceWordWithWordInTernary(clang::ConditionalOperator *ternary)
{
    vector<CXXConstructExpr*> constructExprs;
    HierarchyUtils::getChilds<CXXConstructExpr>(ternary, constructExprs, 1); // depth = 1, only the two immediate expressions

    vector<FixItHint> fixits;
    fixits.reserve(2);
    if (constructExprs.size() != 2) {
        llvm::errs() << "Weird ternary operator with " << constructExprs.size() << " at " << ternary->getLocStart().printToString(sm()) << "\n";
        assert(false);
        return fixits;
    }

    for (int i = 0; i < 2; ++i) {
        SourceLocation rangeStart = constructExprs[i]->getLocStart();
        SourceLocation rangeEnd = Lexer::getLocForEndOfToken(rangeStart, -1, sm(), lo());
        fixits.push_back(FixItHint::CreateReplacement(SourceRange(rangeStart, rangeEnd), "QStringLiteral"));
    }

    return fixits;
}


// true for: QString::fromLatin1().arg()
// false for: QString::fromLatin1("")
// true for: QString s = QString::fromLatin1("foo")
// false for: s += QString::fromLatin1("foo"), etc.
static bool isQStringLiteralCandidate(Stmt *s, ParentMap *map, int currentCall = 0)
{
    if (!s)
        return false;

    MemberExpr *memberExpr = dyn_cast<MemberExpr>(s);
    if (memberExpr)
        return true;

    auto constructExpr = dyn_cast<CXXConstructExpr>(s);
    if (constructExpr && isOfClass(constructExpr, "QString"))
        return true;

    if (Utils::isAssignOperator(dyn_cast<CXXOperatorCallExpr>(s), "QString", "class QLatin1String"))
        return true;

    if (Utils::isAssignOperator(dyn_cast<CXXOperatorCallExpr>(s), "QString", "class QString &&"))
        return true;

    CallExpr *callExpr = dyn_cast<CallExpr>(s);
    StringLiteral *literal = stringLiteralForCall(callExpr);
    CXXOperatorCallExpr *op = dyn_cast<CXXOperatorCallExpr>(s);
    if (op)
        return false;

    if (currentCall > 0 && callExpr) {
        auto fDecl = callExpr->getDirectCallee();
        if (fDecl && betterTakeQLatin1String(dyn_cast<CXXMethodDecl>(fDecl), literal))
            return false;

        return true;
    }

    if (currentCall == 0 || dyn_cast<ImplicitCastExpr>(s) || dyn_cast<CXXBindTemporaryExpr>(s) || dyn_cast<MaterializeTemporaryExpr>(s)) // skip this cruft
        return isQStringLiteralCandidate(HierarchyUtils::parent(map, s), map, currentCall + 1);

    return false;
}

std::vector<FixItHint> QStringUneededHeapAllocations::fixItReplaceFromLatin1OrFromUtf8(CallExpr *callExpr, FromFunction fromFunction)
{
    vector<FixItHint> fixits;

    std::string replacement = isQStringLiteralCandidate(callExpr, m_parentMap) ? "QStringLiteral"
                                                                               : "QLatin1String";

    if (replacement == "QStringLiteral" && callExpr->getLocStart().isMacroID()) {
        queueManualFixitWarning(callExpr->getLocStart(), FromLatin1_FromUtf8Allocations, "Can't use QStringLiteral in macro!");
        return {};
    }

    StringLiteral *literal = stringLiteralForCall(callExpr);
    if (literal) {
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

        auto classNameLoc = Lexer::getLocForEndOfToken(callExpr->getLocStart(), 0, sm(), lo());
        auto scopeOperatorLoc = Lexer::getLocForEndOfToken(classNameLoc, 0, sm(), lo());
        auto methodNameLoc = Lexer::getLocForEndOfToken(scopeOperatorLoc, -1, sm(), lo());
        SourceRange range(callExpr->getLocStart(), methodNameLoc);
        fixits.push_back(FixItHint::CreateReplacement(range, replacement));
    } else {
        queueManualFixitWarning(callExpr->getLocStart(), FromLatin1_FromUtf8Allocations, "Internal error: literal is null");
    }

    return fixits;
}

std::vector<FixItHint> QStringUneededHeapAllocations::fixItRawLiteral(clang::StringLiteral *lt, const string &replacement)
{
    vector<FixItHint> fixits;

    SourceRange range = FixItUtils::rangeForLiteral(m_ci, lt);
    if (range.isInvalid()) {
        if (lt) {
            queueManualFixitWarning(lt->getLocStart(), CharPtrAllocations, "Internal error: Can't calculate source location");
        }
        return {};
    }

    SourceLocation start = lt->getLocStart();
    if (start.isMacroID()) {
        queueManualFixitWarning(start, CharPtrAllocations, "Can't use QStringLiteral in macro..");
    } else {
        string revisedReplacement = lt->getLength() == 0 ? "QLatin1String" : replacement; // QLatin1String("") is better than QStringLiteral("")
        if (revisedReplacement == "QStringLiteral" && lt->getLocStart().isMacroID()) {
            queueManualFixitWarning(lt->getLocStart(), CharPtrAllocations, "Can't use QStringLiteral in macro...");
            return {};
        }

        FixItUtils::insertParentMethodCall(revisedReplacement, range, /**by-ref*/fixits);
    }

    return fixits;
}

void QStringUneededHeapAllocations::VisitOperatorCall(Stmt *stm)
{
    CXXOperatorCallExpr *operatorCall = dyn_cast<CXXOperatorCallExpr>(stm);
    if (!operatorCall)
        return;

    std::vector<StringLiteral*> stringLiterals;
    HierarchyUtils::getChilds<StringLiteral>(operatorCall, stringLiterals);

    //  We're only after string literals, str.contains(some_method_returning_const_char_is_fine())
    if (stringLiterals.empty())
        return;

    FunctionDecl *funcDecl = operatorCall->getDirectCallee();
    if (!funcDecl)
        return;

    CXXMethodDecl *methodDecl = dyn_cast<CXXMethodDecl>(funcDecl);
    if (!isOfClass(methodDecl, "QString"))
        return;

    if (!hasCharPtrArgument(methodDecl))
        return;

    vector<FixItHint> fixits;

    vector<StringLiteral*> literals;
    HierarchyUtils::getChilds<StringLiteral>(stm, literals, 2);

    if (!isOptionSet("no-msvc-compat") && !literals.empty()) {
        if (literals[0]->getNumConcatenated() > 1) {
            return; // Nothing to do here, MSVC doesn't like it
        }
    }

    if (isFixitEnabled(CharPtrAllocations)) {
        if (literals.empty()) {
            queueManualFixitWarning(stm->getLocStart(), CharPtrAllocations, "Couldn't find literal");
        } else {
            const string replacement = Utils::isAscii(literals[0]) ? "QLatin1String" : "QStringLiteral";
            fixits = fixItRawLiteral(literals[0], replacement);
        }
    }

    string msg = string("QString(const char*) being called");
    emitWarning(stm->getLocStart(), msg, fixits);
}

void QStringUneededHeapAllocations::VisitFromLatin1OrUtf8(Stmt *stmt)
{
    CallExpr *callExpr = dyn_cast<CallExpr>(stmt);
    if (!callExpr)
        return;

    FunctionDecl *functionDecl = callExpr->getDirectCallee();
    if (!StringUtils::functionIsOneOf(functionDecl, {"fromLatin1", "fromUtf8"}))
        return;

    CXXMethodDecl *methodDecl = dyn_cast<CXXMethodDecl>(functionDecl);
    if (!isOfClass(methodDecl, "QString"))
        return;

    if (!Utils::callHasDefaultArguments(callExpr) || !hasCharPtrArgument(functionDecl, 2)) // QString::fromLatin1("foo", 1) is ok
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
    HierarchyUtils::getChilds(callExpr, ternaries, 2);
    if (!ternaries.empty()) {
        auto ternary = ternaries[0];
        if (Utils::ternaryOperatorIsOfStringLiteral(ternary)) {
            emitWarning(stmt->getLocStart(), string("QString::fromLatin1() being passed a literal"));
        }

        return;
    }

    std::vector<FixItHint> fixits;

    if (isFixitEnabled(FromLatin1_FromUtf8Allocations)) {
        const FromFunction fromFunction = functionDecl->getNameAsString() == "fromLatin1" ? FromLatin1 : FromUtf8;
        fixits = fixItReplaceFromLatin1OrFromUtf8(callExpr, fromFunction);
    }

    if (functionDecl->getNameAsString() == "fromLatin1") {
        emitWarning(stmt->getLocStart(), string("QString::fromLatin1() being passed a literal"), fixits);
    } else {
        emitWarning(stmt->getLocStart(), string("QString::fromUtf8() being passed a literal"), fixits);
    }
}

void QStringUneededHeapAllocations::VisitAssignOperatorQLatin1String(Stmt *stmt)
{
    CXXOperatorCallExpr *callExpr = dyn_cast<CXXOperatorCallExpr>(stmt);
    if (!Utils::isAssignOperator(callExpr, "QString", "class QLatin1String"))
        return;

    if (!containsStringLiteralNoCallExpr(stmt))
        return;

    ConditionalOperator *ternary = nullptr;
    Stmt *begin = qlatin1CtorExpr(stmt, ternary);

    if (!begin)
        return;

    vector<FixItHint> fixits;

    if (isFixitEnabled(QLatin1StringAllocations)) {
        fixits = ternary == nullptr ? fixItReplaceWordWithWord(begin, "QStringLiteral", "QLatin1String", QLatin1StringAllocations)
                                    : fixItReplaceWordWithWordInTernary(ternary);
    }

    emitWarning(stmt->getLocStart(), string("QString::operator=(QLatin1String(\"literal\")"), fixits);
}

vector<string> QStringUneededHeapAllocations::supportedOptions() const
{
    // no-msvc-compat - use QStringLiteral inside arrays, which is fine if you don't use MSVC

    static const vector<string> options = { "no-msvc-compat" };
    return options;
}

const char *const s_checkName = "qstring-uneeded-heap-allocations";
REGISTER_CHECK_WITH_FLAGS(s_checkName, QStringUneededHeapAllocations, CheckLevel1)
REGISTER_FIXIT(QLatin1StringAllocations, "fix-qlatin1string-allocations", s_checkName)
REGISTER_FIXIT(FromLatin1_FromUtf8Allocations, "fix-fromLatin1_fromUtf8-allocations", s_checkName)
REGISTER_FIXIT(CharPtrAllocations, "fix-fromCharPtrAllocations", s_checkName)
