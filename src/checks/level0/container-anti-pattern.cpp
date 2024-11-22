/*
    SPDX-FileCopyrightText: 2016 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "container-anti-pattern.h"
#include "HierarchyUtils.h"
#include "LoopUtils.h"
#include "StringUtils.h"
#include "Utils.h"
#include "clazy_stl.h"

#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/Stmt.h>
#include <clang/Basic/LLVM.h>
#include <llvm/Support/Casting.h>

#include <vector>

using namespace clang;

ContainerAntiPattern::ContainerAntiPattern(const std::string &name, ClazyContext *context)
    : CheckBase(name, context, Option_CanIgnoreIncludes)
{
}

static bool isInterestingCall(CallExpr *call)
{
    FunctionDecl *func = call ? call->getDirectCallee() : nullptr;
    if (!func) {
        return false;
    }

    static const std::vector<std::string> methods = {
        "QVector::toList",
        "QList::toVector",
        "QMap::values",
        "QMap::keys",
        "QSet::toList",
        "QSet::values",
        "QHash::values",
        "QHash::keys",
        "QList::toList", // In case one has code compiling against Qt6, but Qt5 compat
    };

    return clazy::contains(methods, clazy::qualifiedMethodName(func));
}

void ContainerAntiPattern::VisitStmt(clang::Stmt *stmt)
{
    if (handleLoop(stmt)) { // catch for (auto i : map.values()) and such
        return;
    }

    if (VisitQSet(stmt)) {
        return;
    }

    std::vector<CallExpr *> calls = Utils::callListForChain(dyn_cast<CallExpr>(stmt));
    if (calls.size() < 2) {
        return;
    }

    // For an expression like set.toList().count()...
    CallExpr *callexpr1 = calls[calls.size() - 1]; // ...this would be toList()
    // CallExpr *callexpr2 = calls[calls.size() - 2]; // ...and this would be count()

    if (!isInterestingCall(callexpr1)) {
        return;
    }

    emitWarning(stmt->getBeginLoc(), "allocating an unneeded temporary container");
}

bool ContainerAntiPattern::VisitQSet(Stmt *stmt)
{
    auto *secondCall = dyn_cast<CXXMemberCallExpr>(stmt);
    if (!secondCall || !secondCall->getMethodDecl()) {
        return false;
    }

    CXXMethodDecl *secondMethod = secondCall->getMethodDecl();
    const std::string secondMethodName = clazy::qualifiedMethodName(secondMethod);
    if (secondMethodName != "QSet::isEmpty") {
        return false;
    }

    std::vector<CallExpr *> chainedCalls = Utils::callListForChain(secondCall);
    if (chainedCalls.size() < 2) {
        return false;
    }

    CallExpr *firstCall = chainedCalls[chainedCalls.size() - 1];
    FunctionDecl *firstFunc = firstCall->getDirectCallee();
    if (!firstFunc) {
        return false;
    }

    auto *firstMethod = dyn_cast<CXXMethodDecl>(firstFunc);
    if (!firstMethod || clazy::qualifiedMethodName(firstMethod) != "QSet::intersect") {
        return false;
    }

    emitWarning(stmt->getBeginLoc(), "Use QSet::intersects() instead");
    return true;
}

bool ContainerAntiPattern::handleLoop(Stmt *stm)
{
    Expr *containerExpr = clazy::containerExprForLoop(stm);
    if (!containerExpr) {
        return false;
    }

    auto *memberExpr = clazy::getFirstChildOfType2<CXXMemberCallExpr>(containerExpr);
    if (isInterestingCall(memberExpr)) {
        emitWarning(stm->getBeginLoc(), "allocating an unneeded temporary container");
        return true;
    }

    return false;
}
