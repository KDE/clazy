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
  auto declRefExpr = dyn_cast<DeclRefExpr>(arg);
  declRefExpr =
      declRefExpr ? declRefExpr : clazy::getFirstChildOfType<DeclRefExpr>(arg);
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
        // Is it compile time known QString i.e., not from a function call
        auto qstringCtor = clazy::getFirstChildOfType<CXXConstructExpr>(qstring);
        if (!qstringCtor)
            return false;

        auto *stringLit = clazy::getFirstChildOfType<StringLiteral>(qstringCtor);
        if (!stringLit)
            return false;

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

static bool isTemporaryQRegexObj(Expr *qregexVar, const LangOptions &lo) {
  // Get the QRegularExpression ctor
  auto ctor = clazy::getFirstChildOfType<CXXConstructExpr>(qregexVar);
  if (!ctor || ctor->getNumArgs() == 0) {
      return false;
  }

  // Check if its first arg is "QString"
  auto qstrArg = ctor->getArg(0);
  if (!qstrArg || clazy::typeName(qstrArg->getType(), lo, true) != "QString") {
      return false;
  }

  if (isQStringFromStringLiteral(qstrArg)) {
      return true;
  }
  return false;
}

static bool isQRegexpFromStringLiteral(VarDecl *qregexVarDecl)
{
    Expr *initExpr = getVarInitExpr(qregexVarDecl);
    if (!initExpr) {
        return false;
    }

    auto ctorCall = dyn_cast<CXXConstructExpr>(initExpr);
    if (!ctorCall) {
        ctorCall = clazy::getFirstChildOfType<CXXConstructExpr>(initExpr);
        if (!ctorCall) {
            return false;
        }
    }

    if (ctorCall->getNumArgs() < 2) {
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

static bool isOfAcceptableType(CXXMethodDecl *methodDecl) {
  const auto type = clazy::classNameFor(methodDecl);
  return type == "QString" || type == "QStringList" ||
         type == "QRegularExpression";
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
    if (!methodDecl || !methodDecl->getDeclName().isIdentifier())
        return;

    if (!isOfAcceptableType(methodDecl)) {
      return;
    }

    // QRegularExpression.match()
    const auto methodName = methodDecl->getName();
    if (methodName == "match" || methodName == "globalMatch") {
      auto obj = method->getImplicitObjectArgument()->IgnoreImpCasts();
      if (!obj)
        return;

      if (obj->isLValue()) {
        if (isArgNonStaticLocalVar(obj)) {
          emitWarning(clazy::getLocStart(obj),
                      "Don't create temporary QRegularExpression objects. Use "
                      "a static QRegularExpression object instead");
          return;
        }
      } else if (obj->isXValue()) {
        // is it a temporary?
        auto temp = dyn_cast<MaterializeTemporaryExpr>(obj);
        if (!temp) {
          return;
        }
        if (isTemporaryQRegexObj(temp, lo())) {
          emitWarning(clazy::getLocStart(temp),
                      "Don't create temporary QRegularExpression objects. Use "
                      "a static QRegularExpression object instead");
        }
      }
      return;
    }

    if (!firstArgIsQRegularExpression(methodDecl, lo())) {
        return;
    }

    Expr* qregexArg = method->getArg(0);
    if (!qregexArg) {
        return;
    }

    // Its a QString*().method(QRegularExpression(arg)) ?
    if (auto temp = isArgTemporaryObj(qregexArg)) {
      if (isTemporaryQRegexObj(temp, lo())) {
        emitWarning(clazy::getLocStart(qregexArg),
                    "Don't create temporary QRegularExpression objects. Use a "
                    "static QRegularExpression object instead");
      }
    }

    // Its a local QRegularExpression variable?
    if (isArgNonStaticLocalVar(qregexArg)) {
        emitWarning(clazy::getLocStart(qregexArg), "Don't create temporary QRegularExpression objects. Use a static QRegularExpression object instead");
    }
}
