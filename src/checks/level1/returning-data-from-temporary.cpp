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

#include "returning-data-from-temporary.h"
#include "Utils.h"
#include "HierarchyUtils.h"
#include "QtUtils.h"
#include "TypeUtils.h"
#include "checkmanager.h"

#include <clang/AST/AST.h>

using namespace clang;
using namespace std;


ReturningDataFromTemporary::ReturningDataFromTemporary(const std::string &name, ClazyContext *context)
    : CheckBase(name, context)
{
}


void ReturningDataFromTemporary::VisitStmt(clang::Stmt *stmt)
{
    auto returnStmt = dyn_cast<ReturnStmt>(stmt);
    if (!returnStmt)
        return;

    CXXMemberCallExpr *memberCall = HierarchyUtils::unpeal<CXXMemberCallExpr>(HierarchyUtils::getFirstChild(returnStmt), HierarchyUtils::IgnoreExprWithCleanups |
                                                                                                                         HierarchyUtils::IgnoreImplicitCasts);
    if (!memberCall)
        return;

    CXXMethodDecl *method = memberCall->getMethodDecl();
    if (!method)
        return;
    const auto methodName = method->getQualifiedNameAsString();

    if (methodName == "QByteArray::data" || methodName == "QByteArray::operator const char *") {
        handleDataCall(memberCall);
    } else if (methodName == "QByteArray::constData") {
        handleConstDataCall();
    }
}

void ReturningDataFromTemporary::handleDataCall(CXXMemberCallExpr *memberCall)
{
    // Handles:
    // return myLocalByteArray.data();
    // return myTemporaryByteArray().data();

    // Doesn't care about constData() calls, since the byte array might be shared.

    Expr *obj = memberCall->getImplicitObjectArgument();
    Stmt *t = obj;
    DeclRefExpr *declRef = nullptr;
    CXXBindTemporaryExpr *temporaryExpr = nullptr;

    while (t) {
        if (dyn_cast<ImplicitCastExpr>(t) || dyn_cast<MaterializeTemporaryExpr>(t)) {
            t = HierarchyUtils::getFirstChild(t);
            continue;
        }

        declRef = dyn_cast<DeclRefExpr>(t);
        if (declRef)
            break;

        temporaryExpr = dyn_cast<CXXBindTemporaryExpr>(t);
        if (temporaryExpr)
            break;

        break;
    }

    if (!temporaryExpr && !declRef)
        return;

    if (declRef) {
        VarDecl *varDecl = dyn_cast<VarDecl>(declRef->getDecl());
        if (!varDecl || varDecl->isStaticLocal() || TypeUtils::valueIsConst(varDecl->getType()))
            return;
    } else if (temporaryExpr) {
        if (TypeUtils::valueIsConst(temporaryExpr->getType()))
            return;
    }

    emitWarning(memberCall, "Returning data of local QByteArray");
}

void ReturningDataFromTemporary::handleConstDataCall()
{
    // TODO
}

REGISTER_CHECK("returning-data-from-temporary", ReturningDataFromTemporary, CheckLevel1)
