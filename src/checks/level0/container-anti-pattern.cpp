/*
    This file is part of the clazy static checker.

    Copyright (C) 2016 Sergio Martins <smartins@kde.org>

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

#include "container-anti-pattern.h"
#include "Utils.h"
#include "StringUtils.h"
#include "LoopUtils.h"
#include "HierarchyUtils.h"
#include "SourceCompatibilityHelpers.h"
#include "clazy_stl.h"

#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/Stmt.h>
#include <clang/Basic/LLVM.h>
#include <llvm/Support/Casting.h>

#include <vector>

class ClazyContext;

using namespace clang;
using namespace std;


ContainerAntiPattern::ContainerAntiPattern(const std::string &name, ClazyContext *context)
    : CheckBase(name, context, Option_CanIgnoreIncludes)
{
}

static bool isInterestingCall(CallExpr *call)
{
    FunctionDecl *func = call ? call->getDirectCallee() : nullptr;
    if (!func)
        return false;

    static const vector<string> methods = { "QVector::toList", "QList::toVector", "QMap::values",
                                            "QMap::keys", "QSet::toList", "QSet::values",
                                            "QHash::values", "QHash::keys" };

    return clazy::contains(methods, clazy::qualifiedMethodName(func));
}

void ContainerAntiPattern::VisitStmt(clang::Stmt *stmt)
{
    if (handleLoop(stmt)) // catch for (auto i : map.values()) and such
        return;

    if (VisitQSet(stmt))
        return;

    vector<CallExpr *> calls = Utils::callListForChain(dyn_cast<CallExpr>(stmt));
    if (calls.size() < 2)
        return;

    // For an expression like set.toList().count()...
    CallExpr *callexpr1 = calls[calls.size() - 1]; // ...this would be toList()
    // CallExpr *callexpr2 = calls[calls.size() - 2]; // ...and this would be count()

    if (!isInterestingCall(callexpr1))
        return;

    emitWarning(clazy::getLocStart(stmt), "allocating an unneeded temporary container");
}

bool ContainerAntiPattern::VisitQSet(Stmt *stmt)
{
    CXXMemberCallExpr *secondCall = dyn_cast<CXXMemberCallExpr>(stmt);
    if (!secondCall || !secondCall->getMethodDecl())
        return false;

    CXXMethodDecl *secondMethod = secondCall->getMethodDecl();
    const string secondMethodName = clazy::qualifiedMethodName(secondMethod);
    if (secondMethodName != "QSet::isEmpty")
        return false;

    vector<CallExpr*> chainedCalls = Utils::callListForChain(secondCall);
    if (chainedCalls.size() < 2)
        return false;

    CallExpr *firstCall = chainedCalls[chainedCalls.size() - 1];
    FunctionDecl *firstFunc = firstCall->getDirectCallee();
    if (!firstFunc)
        return false;

    CXXMethodDecl *firstMethod = dyn_cast<CXXMethodDecl>(firstFunc);
    if (!firstMethod || clazy::qualifiedMethodName(firstMethod) != "QSet::intersect")
        return false;

    emitWarning(clazy::getLocStart(stmt), "Use QSet::intersects() instead");
    return true;
}

bool ContainerAntiPattern::handleLoop(Stmt *stm)
{
    Expr *containerExpr = clazy::containerExprForLoop(stm);
    if (!containerExpr)
        return false;

    auto memberExpr = clazy::getFirstChildOfType2<CXXMemberCallExpr>(containerExpr);
    if (isInterestingCall(memberExpr)) {
        emitWarning(clazy::getLocStart(stm), "allocating an unneeded temporary container");
        return true;
    }

    return false;
}
