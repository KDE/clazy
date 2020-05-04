/*
  This file is part of the clazy static checker.

  Copyright (C) 2020 Sergio Martins <smartins@kde.org>

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

#include "qstring-comparison-to-implicit-char.h"
#include "Utils.h"
#include "HierarchyUtils.h"
#include "QtUtils.h"
#include "TypeUtils.h"

#include <clang/AST/AST.h>

using namespace clang;
using namespace std;


QStringComparisonToImplicitChar::QStringComparisonToImplicitChar(const std::string &name, ClazyContext *context)
    : CheckBase(name, context)
{
}

void QStringComparisonToImplicitChar::VisitStmt(clang::Stmt *stmt)
{
    auto callExpr = dyn_cast<CXXOperatorCallExpr>(stmt);
    if (!callExpr || !callExpr->getCalleeDecl() || callExpr->getNumArgs() != 2)
        return;

    Expr *arg1 = callExpr->getArg(1);
    IntegerLiteral *il = clazy::getFirstChildOfType2<IntegerLiteral>(arg1);
    if (!il)
        return;

    auto functionDecl = dyn_cast<FunctionDecl>(callExpr->getCalleeDecl());
    if (!functionDecl || functionDecl->getQualifiedNameAsString() != "operator==")
        return;

    ParmVarDecl *parm1 = functionDecl->getParamDecl(0);
    if (parm1->getType().getAsString() != "const class QString &")
        return;

    ParmVarDecl *parm2 = functionDecl->getParamDecl(1);
    if (parm2->getType().getAsString() != "class QChar")
        return;


    emitWarning(stmt, "QString being compared to implicit QChar");
}
