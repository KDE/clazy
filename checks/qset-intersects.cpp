/*
   This file is part of the clang-lazy static checker.

  Copyright (C) 2015 Sergio Martins <smartins@kde.org>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

  As a special exception, permission is given to link this program
  with any edition of Qt, and distribute the resulting executable,
  without including the source code for Qt in the source distribution.
*/

#include "qset-intersects.h"
#include "Utils.h"
#include "checkmanager.h"
#include "StringUtils.h"

#include <clang/AST/AST.h>
#include <clang/Lex/Lexer.h>

using namespace clang;
using namespace std;


QSetIntersects::QSetIntersects(const std::string &name, const clang::CompilerInstance &ci)
    : CheckBase(name, ci)
{
}

void QSetIntersects::VisitStmt(clang::Stmt *stmt)
{
    CXXMemberCallExpr *secondCall = dyn_cast<CXXMemberCallExpr>(stmt);
    if (!secondCall || !secondCall->getMethodDecl())
        return;

    CXXMethodDecl *secondMethod = secondCall->getMethodDecl();
    const string secondMethodName = StringUtils::qualifiedMethodName(secondMethod);
    if (secondMethodName != "QSet::isEmpty")
        return;

    vector<CallExpr*> chainedCalls = Utils::callListForChain(secondCall);
    if (chainedCalls.size() < 2)
        return;

    CallExpr *firstCall = chainedCalls[chainedCalls.size() - 1];
    FunctionDecl *firstFunc = firstCall->getDirectCallee();
    if (!firstFunc)
        return;

    CXXMethodDecl *firstMethod = dyn_cast<CXXMethodDecl>(firstFunc);
    if (!firstMethod || StringUtils::qualifiedMethodName(firstMethod) != "QSet::intersect")
        return;

    emitWarning(stmt->getLocStart(), "Use QSet::intersects() instead");
}


REGISTER_CHECK_WITH_FLAGS("qset-intersects", QSetIntersects, HiddenCheckLevel) // TODO: Once Qt 5.6 is released, move to CheckLevel0
