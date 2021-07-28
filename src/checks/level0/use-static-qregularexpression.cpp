/*
  This file is part of the clazy static checker.

  Copyright (C) 2021 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
  Author: Waqar Ahmed <waqar.ahmed@kdab.com>

  Copyright (C) 2021 Waqar Ahmed <waqar.17a@gmail.com>

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

#include "use-static-qregularexpression.h"
#include "Utils.h"
#include "HierarchyUtils.h"
#include "QtUtils.h"
#include "TypeUtils.h"

#include <clang/AST/AST.h>
#include <clang/AST/ExprCXX.h>

using namespace clang;
using namespace std;

UseStaticQRegularExpression::UseStaticQRegularExpression(const std::string &name, ClazyContext *context)
    : CheckBase(name, context)
{
}

static MaterializeTemporaryExpr* isArgTemporaryObj(Expr *arg0)
{
    return dyn_cast_or_null<MaterializeTemporaryExpr>(arg0);
}

static VarDecl* getVarDecl(Expr *arg)
{
    auto declRefExpr = dyn_cast_or_null<DeclRefExpr>(clazy::getFirstChild(arg));
    if (!declRefExpr) {
        return nullptr;
    }
    return dyn_cast_or_null<VarDecl>(declRefExpr->getDecl());
}

static Expr* getVarInitExpr(VarDecl *VDef)
{
    return VDef->getDefinition() ? VDef->getDefinition()->getInit() : nullptr;
}

static bool isQStringFromStringLiteral(Expr *qstring)
{
    if (isArgTemporaryObj(qstring)) {
        return true;
    }

    if (auto *VD = getVarDecl(qstring)) {
        auto *stringLit = clazy::getFirstChildOfType<StringLiteral>(getVarInitExpr(VD));
        if (stringLit) {
            return true;
        }
    }
    return false;
}

static bool isQRegexpFromStringLiteral(VarDecl *qregexVarDecl)
{
    Expr *initExpr = getVarInitExpr(qregexVarDecl);
    if (!initExpr) {
        return false;
    }

    auto ctorCall = dyn_cast_or_null<CXXConstructExpr>(*initExpr->child_begin());
    if (!ctorCall) {
        return false;
    }

    auto qstringArg = ctorCall->getArg(0);
    if (qstringArg && isQStringFromStringLiteral(qstringArg)) {
        return true;
    }

    return false;
}

static bool isArgNonStaticLocalVar(Expr *qregexp)
{
    auto *varDecl = getVarDecl(qregexp);
    if (!varDecl) {
        return false;
    }

    if (!isQRegexpFromStringLiteral(varDecl)) {
        return false;
    }

    return varDecl->isLocalVarDecl() && !varDecl->isStaticLocal();
}

static bool isQStringOrQStringListMethod(CXXMethodDecl *methodDecl)
{
    return clazy::isOfClass(methodDecl, "QString") || clazy::isOfClass(methodDecl, "QStringList");
}

static bool firstArgIsQRegularExpression(CXXMethodDecl* methodDecl, const LangOptions& lo)
{
    return clazy::simpleArgTypeName(methodDecl, 0, lo) == "QRegularExpression";
}

void UseStaticQRegularExpression::VisitStmt(clang::Stmt *stmt)
{
    if (!stmt) {
        return;
    }

    auto method = dyn_cast_or_null<CXXMemberCallExpr>(stmt);
    if (!method) {
        return;
    }

    if (method->getNumArgs() == 0) {
        return;
    }

    auto methodDecl = method->getMethodDecl();
    if (!isQStringOrQStringListMethod(methodDecl)) {
        return;
    }
    if (!firstArgIsQRegularExpression(methodDecl, lo())) {
        return;
    }

    Expr* qregexArg = method->getArg(0);
    if (!qregexArg) {
        return;
    }

    // Its a QRegularExpression(arg) ?
    if (auto temp = isArgTemporaryObj(qregexArg)) {
        // Get the QRegularExpression ctor
        auto ctor = clazy::getFirstChildOfType<CXXConstructExpr>(temp);

        // Check if its first arg is "QString" && a temporary OR non-static local
        auto qstrArg = ctor->getArg(0);
        if (!qstrArg || clazy::typeName(qstrArg->getType(), lo(), true) != "QString") {
            return;
        }

        if (isQStringFromStringLiteral(qstrArg)) {
            emitWarning(clazy::getLocStart(qregexArg), "Don't create temporary QRegularExpression objects. Use a static QRegularExpression object instead");
        }
        return;
    }

    // Its a local QRegularExpression variable?
    if (isArgNonStaticLocalVar(qregexArg)) {
        emitWarning(clazy::getLocStart(qregexArg), "Don't create temporary QRegularExpression objects. Use a static QRegularExpression object instead");
        return;
    }
}
