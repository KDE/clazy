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

#include "returning-void-expression.h"
#include "Utils.h"
#include "HierarchyUtils.h"
#include "ContextUtils.h"
#include "QtUtils.h"
#include "TypeUtils.h"
#include "checkmanager.h"

#include <clang/AST/AST.h>

using namespace clang;
using namespace std;


ReturningVoidExpression::ReturningVoidExpression(const std::string &name, const clang::CompilerInstance &ci)
    : CheckBase(name, ci)
{
}

void ReturningVoidExpression::VisitStmt(clang::Stmt *stmt)
{
    auto ret = dyn_cast<ReturnStmt>(stmt);
    if (!ret || !clazy_std::hasChildren(ret))
        return;

    QualType qt = ret->getRetValue()->getType();
    if (qt.isNull() || !qt->isVoidType())
        return;

    DeclContext *context = ContextUtils::contextForDecl(m_lastDecl);
    if (!context)
        return;

    auto func = dyn_cast<FunctionDecl>(context);
    // A function template returning T won't bailout in the void check above, do it properly now:
    if (!func || !func->getReturnType()->isVoidType())
        return;

    emitWarning(stmt, "Returning a void expression");
}

REGISTER_CHECK_WITH_FLAGS("returning-void-expression", ReturningVoidExpression, CheckLevel2)
