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
#include "checkmanager.h"
#include "StringUtils.h"
#include "MacroUtils.h"

#include <clang/AST/AST.h>
#include <clang/Lex/Lexer.h>

using namespace clang;
using namespace std;


ContainerAntiPattern::ContainerAntiPattern(const std::string &name, const clang::CompilerInstance &ci)
    : CheckBase(name, ci)
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

    return clazy_std::contains(methods, StringUtils::qualifiedMethodName(func));
}

void ContainerAntiPattern::VisitStmt(clang::Stmt *stmt)
{
    if (handleRangeLoop(dyn_cast<CXXForRangeStmt>(stmt)))
        return;

    if (handleForeach(dyn_cast<CXXConstructExpr>(stmt)))
        return;

    vector<CallExpr *> calls = Utils::callListForChain(dyn_cast<CallExpr>(stmt));
    if (calls.size() < 2)
        return;

    // For an expression like set.toList().count()...
    CallExpr *callexpr1 = calls[calls.size() - 1]; // ...this would be toList()
    // CallExpr *callexpr2 = calls[calls.size() - 2]; // ...and this would be count()

    if (!isInterestingCall(callexpr1))
        return;


    emitWarning(stmt->getLocStart(), "allocating an unneeded temporary container");
}

bool ContainerAntiPattern::handleRangeLoop(CXXForRangeStmt *stm)
{
    Expr *containerExpr = stm ? stm->getRangeInit() : nullptr;
    if (!containerExpr)
        return false;

    auto memberExpr = HierarchyUtils::getFirstChildOfType2<CXXMemberCallExpr>(containerExpr);
    if (isInterestingCall(memberExpr)) {
        emitWarning(stm->getLocStart(), "allocating an unneeded temporary container");
        return true;
    }

    return false;
}

bool ContainerAntiPattern::handleForeach(clang::CXXConstructExpr *constructExpr)
{
    if (!constructExpr || constructExpr->getNumArgs() < 1)
        return false;

    CXXConstructorDecl *constructorDecl = constructExpr->getConstructor();
    if (!constructorDecl || constructorDecl->getNameAsString() != "QForeachContainer")
        return false;

    auto memberExpr = HierarchyUtils::getFirstChildOfType2<CXXMemberCallExpr>(constructExpr);
    if (isInterestingCall(memberExpr)) {
        emitWarning(constructExpr->getLocStart(), "allocating an unneeded temporary container");
        return true;
    }

    return false;
}


REGISTER_CHECK_WITH_FLAGS("container-anti-pattern", ContainerAntiPattern, CheckLevel0)
