/**********************************************************************
**  Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
**  Author: Sérgio Martins <sergio.martins@kdab.com>
**
** This file may be distributed and/or modified under the terms of the
** GNU Lesser General Public License version 2.1 and version 3 as published by the
** Free Software Foundation and appearing in the file LICENSE.LGPL.txt included.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**********************************************************************/

#include "qstringuneededheapallocations.h"
#include "Utils.h"
#include "StringUtils.h"
#include "MethodSignatureUtils.h"

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

QStringUneededHeapAllocations::QStringUneededHeapAllocations(clang::CompilerInstance &ci)
    : CheckBase(ci)
{
}

void QStringUneededHeapAllocations::VisitStmt(clang::Stmt *stm)
{
    VisitCtor(stm);
    VisitOperatorCall(stm);
    VisitFromLatin1OrUtf8(stm);
    VisitAssignOperatorQLatin1String(stm);
}

std::string QStringUneededHeapAllocations::name() const
{
    return "qstring-uneeded-heap-allocations";
}

// Returns the first occurrence of a QLatin1String(char*) CTOR call
static Stmt *qlatin1CtorExpr(Stmt *stm, ConditionalOperator * &ternary)
{
    if (stm == nullptr)
        return nullptr;

    CXXConstructExpr *constructExpr = dyn_cast<CXXConstructExpr>(stm);
    if (constructExpr != nullptr) {
        CXXConstructorDecl *ctor = constructExpr->getConstructor();
        if (isOfClass(ctor, "QLatin1String") && hasCharPtrArgument(ctor, 1)) {
            if (Utils::containsStringLiteral(constructExpr, /*allowEmpty=*/ false, 2))
                return constructExpr;
        }
    }

    if (ternary == nullptr) {
        ternary = dyn_cast<ConditionalOperator>(stm);
    }

    auto it = stm->child_begin();
    auto end = stm->child_end();

    for (; it != end; ++it) {
        auto expr = qlatin1CtorExpr(*it, ternary);
        if (expr != nullptr)
            return expr;
    }

    return nullptr;
}

// Returns true if there's a literal in the hierarchy, but aborts if it's parented on CallExpr
// so, returns true for: QLatin1String("foo") but false for QLatin1String(indirection("foo"));
//
static bool containsStringLiteralNoCallExpr(Stmt *stmt)
{
    if (stmt == nullptr)
        return false;

    StringLiteral *sl = dyn_cast<StringLiteral>(stmt);
    if (sl != nullptr)
        return true;

    auto it = stmt->child_begin();
    auto end = stmt->child_end();

    for (; it != end; ++it) {
        if (*it == nullptr)
            continue;
        CallExpr *callExpr = dyn_cast<CallExpr>(*it);
        if (callExpr)
            continue;

        if (containsStringLiteralNoCallExpr(*it))
            return true;
    }

    return false;
}

// For QString::fromLatin1("foo") returns "foo"
static StringLiteral* stringLiteralForCall(CallExpr *call)
{
    if (!call)
        return nullptr;

    vector<StringLiteral*> literals;
    Utils::getChilds2(call, literals, 2);
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
        Stmt *begin = qlatin1CtorExpr(stm, ternary);
        if (begin == nullptr)
            return;

        vector<FixItHint> fixits = ternary == nullptr ? fixItReplaceQLatin1StringWithQStringLiteral(begin)
                                                      : fixItReplaceQLatin1StringWithQStringLiteralInTernary(ternary);

        emitWarning(stm->getLocStart(), msg, fixits);
    } else {
        emitWarning(stm->getLocStart(), msg);
    }
}

vector<FixItHint> QStringUneededHeapAllocations::fixItReplaceQLatin1StringWithQStringLiteral(clang::Stmt *begin)
{
    vector<FixItHint> fixits;
    SourceLocation rangeStart = begin->getLocStart();
    SourceLocation rangeEnd = Lexer::getLocForEndOfToken(rangeStart, -1, m_ci.getSourceManager(), m_ci.getLangOpts());
    fixits.push_back(FixItHint::CreateReplacement(SourceRange(rangeStart, rangeEnd), "QStringLiteral"));
    return fixits;
}

vector<FixItHint> QStringUneededHeapAllocations::fixItReplaceQLatin1StringWithQStringLiteralInTernary(clang::ConditionalOperator *ternary)
{
    vector<CXXConstructExpr*> constructExprs;
    Utils::getChilds2<CXXConstructExpr>(ternary, constructExprs, 1); // depth = 1, only the two immediate expressions

    vector<FixItHint> fixits;
    fixits.reserve(2);
    if (constructExprs.size() != 2) {
        llvm::errs() << "Weird ternary operator with " << constructExprs.size() << " at " << ternary->getLocStart().printToString(m_ci.getSourceManager()) << "\n";
        assert(false);
        return fixits;
    }

    for (int i = 0; i < 2; ++i) {
        SourceLocation rangeStart = constructExprs[i]->getLocStart();
        SourceLocation rangeEnd = Lexer::getLocForEndOfToken(rangeStart, -1, m_ci.getSourceManager(), m_ci.getLangOpts());
        fixits.push_back(FixItHint::CreateReplacement(SourceRange(rangeStart, rangeEnd), "QStringLiteral"));
    }

    return fixits;}

std::vector<FixItHint> QStringUneededHeapAllocations::fixItReplaceFromLatin1OrFromUtf8(CallExpr *callExpr)
{
    Stmt *grandParent = Utils::parent(m_parentMap, callExpr, 2);
    if (grandParent == nullptr)
        return {};

    vector<FixItHint> fixits;

    ImplicitCastExpr *implicitCastExpr = dyn_cast<ImplicitCastExpr>(grandParent);

    bool toStringLiteralForSure = false;
    if (implicitCastExpr) {
        // QString::fromLatin1("foo % 1").arg(foo) can only be replaced by QStringLiteral, due to arg() call
        // to detect that, we check if the grandparent is an implicit cast to QString
        auto record = implicitCastExpr->getBestDynamicClassType();
        if (record && record->getNameAsString() == "QString")
            toStringLiteralForSure = true;
    }

    if (toStringLiteralForSure) {
        StringLiteral *literal = stringLiteralForCall(callExpr);
        if (literal) {
            SourceRange range(callExpr->getLocStart(), literal->getLocStart().getLocWithOffset(-2));
            fixits.push_back(FixItHint::CreateReplacement(range, "QStringLiteral"));
        } else {
            llvm::errs() << "Failed to apply fixit for location: ";
            StringUtils::printLocation(callExpr);
            assert(false);
        }
    }

    return fixits;
}

void QStringUneededHeapAllocations::VisitOperatorCall(Stmt *stm)
{
    CXXOperatorCallExpr *operatorCall = dyn_cast<CXXOperatorCallExpr>(stm);
    if (operatorCall == nullptr)
        return;

    std::vector<StringLiteral*> stringLiterals;
    Utils::getChilds2<StringLiteral>(operatorCall, stringLiterals);

    //  We're only after string literals, str.contains(some_method_returning_const_char_is_fine())
    if (stringLiterals.empty())
        return;

    FunctionDecl *funcDecl = operatorCall->getDirectCallee();
    if (funcDecl == nullptr)
        return;

    CXXMethodDecl *methodDecl = dyn_cast<CXXMethodDecl>(funcDecl);
    if (!isOfClass(methodDecl, "QString"))
        return;

    if (!hasCharPtrArgument(methodDecl))
        return;

    string msg = string("QString(const char*) being called");
    emitWarning(stm->getLocStart(), msg);
}

void QStringUneededHeapAllocations::VisitFromLatin1OrUtf8(Stmt *stmt)
{
    CallExpr *callExpr = dyn_cast<CallExpr>(stmt);
    if (callExpr == nullptr)
        return;

    FunctionDecl *functionDecl = callExpr->getDirectCallee();
    if (functionDecl == nullptr)
        return;

    CXXMethodDecl *methodDecl = dyn_cast<CXXMethodDecl>(functionDecl);
    if (methodDecl == nullptr)
        return;

    std::string functionName = functionDecl->getNameAsString();
    if (functionName != "fromLatin1" && functionName != "fromUtf8")
        return;

    if (!isOfClass(methodDecl, "QString"))
        return;

    if (!Utils::callHasDefaultArguments(callExpr) || !hasCharPtrArgument(functionDecl, 2)) // QString::fromLatin1("foo", 1) is ok
        return;

    if (!containsStringLiteralNoCallExpr(callExpr))
        return;

    std::vector<FixItHint> fixits = fixItReplaceFromLatin1OrFromUtf8(callExpr);

    if (functionName == "fromLatin1") {
        emitWarning(stmt->getLocStart(), string("QString::fromLatin1() being passed a literal"), fixits);
    } else {
        emitWarning(stmt->getLocStart(), string("QString::fromUtf8() being passed a literal"), fixits);
    }
}

void QStringUneededHeapAllocations::VisitAssignOperatorQLatin1String(Stmt *stmt)
{
    CXXOperatorCallExpr *callExpr = dyn_cast<CXXOperatorCallExpr>(stmt);
    if (callExpr == nullptr)
        return;

    FunctionDecl *functionDecl = callExpr->getDirectCallee();
    if (functionDecl == nullptr)
        return;

    CXXMethodDecl *methodDecl = dyn_cast<CXXMethodDecl>(functionDecl);
    if (!isOfClass(methodDecl, "QString") || functionDecl->getNameAsString() != "operator=" || !hasArgumentOfType(functionDecl, "class QLatin1String", 1))
        return;

    if (!containsStringLiteralNoCallExpr(stmt))
        return;

    ConditionalOperator *ternary = nullptr;
    Stmt *begin = qlatin1CtorExpr(stmt, ternary);

    if (begin == nullptr)
        return;

    vector<FixItHint> fixits = ternary == nullptr ? fixItReplaceQLatin1StringWithQStringLiteral(begin)
                                                  : fixItReplaceQLatin1StringWithQStringLiteralInTernary(ternary);

    emitWarning(stmt->getLocStart(), string("QString::operator=(QLatin1String(\"literal\")"), fixits);
}
