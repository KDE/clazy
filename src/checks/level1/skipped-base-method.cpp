/*
  This file is part of the clazy static checker.

    Copyright (C) 2017 Sergio Martins <smartins@kde.org>

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

#include "skipped-base-method.h"
#include "HierarchyUtils.h"
#include "TypeUtils.h"
#include "FunctionUtils.h"

#include <clang/AST/DeclCXX.h>
#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/Type.h>
#include <clang/Basic/LLVM.h>
#include <llvm/Support/Casting.h>

#include <vector>

class ClazyContext;

using namespace clang;
using namespace std;


SkippedBaseMethod::SkippedBaseMethod(const std::string &name, ClazyContext *context)
    : CheckBase(name, context)
{
}

void SkippedBaseMethod::VisitStmt(clang::Stmt *stmt)
{
    auto memberCall = dyn_cast<CXXMemberCallExpr>(stmt);
    if (!memberCall)
        return;

    auto expr = memberCall->getImplicitObjectArgument();
    auto thisExpr = clazy::unpeal<CXXThisExpr>(expr, clazy::IgnoreImplicitCasts);
    if (!thisExpr)
        return;

    const CXXRecordDecl *thisClass = thisExpr->getType()->getPointeeCXXRecordDecl();
    const CXXRecordDecl *baseClass = memberCall->getRecordDecl();

    std::vector<CXXRecordDecl*> baseClasses;
    if (!clazy::derivesFrom(thisClass, baseClass, &baseClasses) || baseClasses.size() < 2)
        return;

    // We're calling a grand-base method, so check if a more direct base also implements it
    for (int i = baseClasses.size() - 1; i > 0; --i) { // the higher indexes have the most derived classes
        CXXRecordDecl *moreDirectBaseClass = baseClasses[i];
        if (clazy::classImplementsMethod(moreDirectBaseClass, memberCall->getMethodDecl())) {
            std::string msg = "Maybe you meant to call " + moreDirectBaseClass->getNameAsString() + "::" + memberCall->getMethodDecl()->getNameAsString() + "() instead";
            emitWarning(stmt, msg);
        }
    }
}
