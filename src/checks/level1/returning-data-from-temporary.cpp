/*
  This file is part of the clazy static checker.

    Copyright (C) 2016-2017 Sergio Martins <smartins@kde.org>

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
#include "HierarchyUtils.h"
#include "TypeUtils.h"

#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/Type.h>
#include <clang/Basic/LLVM.h>
#include <llvm/Support/Casting.h>

class ClazyContext;

using namespace clang;
using namespace std;


ReturningDataFromTemporary::ReturningDataFromTemporary(const std::string &name, ClazyContext *context)
    : CheckBase(name, context, Option_CanIgnoreIncludes)
{
}

void ReturningDataFromTemporary::VisitStmt(clang::Stmt *stmt)
{
    if (handleReturn(dyn_cast<ReturnStmt>(stmt)))
        return;

    handleDeclStmt(dyn_cast<DeclStmt>(stmt));
}

bool ReturningDataFromTemporary::handleReturn(ReturnStmt *ret)
{
    if (!ret)
        return false;

    auto memberCall = clazy::unpeal<CXXMemberCallExpr>(clazy::getFirstChild(ret), clazy::IgnoreExprWithCleanups |
                                                       clazy::IgnoreImplicitCasts);
    handleMemberCall(memberCall, false);
    return true;
}

void ReturningDataFromTemporary::handleDeclStmt(DeclStmt *declStmt)
{
    if (!declStmt)
        return;

    for (auto decl : declStmt->decls()) {
        auto varDecl = dyn_cast<VarDecl>(decl);
        if (!varDecl)
            continue;

        if (varDecl->getType().getAsString() != "const char *")
            continue;

        Expr *init = varDecl->getInit();
        if (!init)
            continue;

        auto memberCall = clazy::unpeal<CXXMemberCallExpr>(clazy::getFirstChild(init), clazy::IgnoreExprWithCleanups |
                                                           clazy::IgnoreImplicitCasts);


        handleMemberCall(memberCall, true);
    }
}

void ReturningDataFromTemporary::handleMemberCall(CXXMemberCallExpr *memberCall, bool onlyTemporaries)
{
    if (!memberCall)
        return;

    CXXMethodDecl *method = memberCall->getMethodDecl();
    if (!method)
        return;
    const auto methodName = method->getQualifiedNameAsString();

    if (methodName != "QByteArray::data" &&
        methodName != "QByteArray::operator const char *" &&
        methodName != "QByteArray::constData")
        return;


    Expr *obj = memberCall->getImplicitObjectArgument();
    Stmt *t = obj;
    DeclRefExpr *declRef = nullptr;
    CXXBindTemporaryExpr *temporaryExpr = nullptr;

    while (t) {
        if (dyn_cast<ImplicitCastExpr>(t) || dyn_cast<MaterializeTemporaryExpr>(t)) {
            t = clazy::getFirstChild(t);
            continue;
        }

        if (!onlyTemporaries) {
            declRef = dyn_cast<DeclRefExpr>(t);
            if (declRef)
                break;
        }

        temporaryExpr = dyn_cast<CXXBindTemporaryExpr>(t);
        if (temporaryExpr)
            break;

        break;
    }

    if (!temporaryExpr && !declRef)
        return;

    if (declRef) {
        VarDecl *varDecl = dyn_cast<VarDecl>(declRef->getDecl());
        if (!varDecl || varDecl->isStaticLocal() || clazy::valueIsConst(varDecl->getType()))
            return;

        QualType qt = varDecl->getType();
        if (qt.isNull() || qt->isReferenceType())
            return;
    } else if (temporaryExpr) {
        if (clazy::valueIsConst(temporaryExpr->getType()))
            return;
    }

    emitWarning(memberCall, "Returning data of temporary QByteArray");
}
