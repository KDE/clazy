/*
  This file is part of the clazy static checker.

  SPDX-FileCopyrightText: 2020 Sergio Martins <smartins@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "qstring-comparison-to-implicit-char.h"
#include "HierarchyUtils.h"
#include "QtUtils.h"
#include "TypeUtils.h"
#include "Utils.h"

#include <clang/AST/AST.h>

using namespace clang;

QStringComparisonToImplicitChar::QStringComparisonToImplicitChar(const std::string &name, ClazyContext *context)
    : CheckBase(name, context)
{
}

void QStringComparisonToImplicitChar::VisitStmt(clang::Stmt *stmt)
{
    auto *callExpr = dyn_cast<CXXOperatorCallExpr>(stmt);
    if (!callExpr || !callExpr->getCalleeDecl() || callExpr->getNumArgs() != 2) {
        return;
    }

    Expr *arg1 = callExpr->getArg(1);
    auto *il = clazy::getFirstChildOfType2<IntegerLiteral>(arg1);
    if (!il) {
        return;
    }

    auto *functionDecl = dyn_cast<FunctionDecl>(callExpr->getCalleeDecl());
    if (!functionDecl || functionDecl->getQualifiedNameAsString() != "operator==") {
        return;
    }

    ParmVarDecl *parm1 = functionDecl->getParamDecl(0);
    if (parm1->getType().getAsString() != "const class QString &") {
        return;
    }

    ParmVarDecl *parm2 = functionDecl->getParamDecl(1);
    if (parm2->getType().getAsString() != "class QChar") {
        return;
    }

    emitWarning(stmt, "QString being compared to implicit QChar");
}
