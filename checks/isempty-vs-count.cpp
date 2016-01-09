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

#include "isempty-vs-count.h"
#include "Utils.h"
#include "checkmanager.h"
#include "StringUtils.h"
#include "QtUtils.h"

#include <clang/AST/AST.h>
#include <clang/Lex/Lexer.h>

using namespace clang;
using namespace std;


IsEmptyVSCount::IsEmptyVSCount(const std::string &name, const clang::CompilerInstance &ci)
    : CheckBase(name, ci)
{
}

void IsEmptyVSCount::VisitStmt(clang::Stmt *stmt)
{
    ImplicitCastExpr *cast = dyn_cast<ImplicitCastExpr>(stmt);
    if (!cast || cast->getCastKind() != clang::CK_IntegralToBoolean)
        return;

    CXXMemberCallExpr *memberCall = dyn_cast<CXXMemberCallExpr>(*(cast->child_begin()));
    CXXMethodDecl *method = memberCall ? memberCall->getMethodDecl() : nullptr;

    if (!StringUtils::functionIsOneOf(method, {"size", "count", "length"}))
        return;

    if (!StringUtils::classIsOneOf(method->getParent(), QtUtils::qtContainers()))
        return;

    emitWarning(stmt->getLocStart(), "use isEmpty() instead");
}


REGISTER_CHECK_WITH_FLAGS("isempty-vs-count", IsEmptyVSCount, CheckLevel2)
