/*
    SPDX-FileCopyrightText: 2017 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "skipped-base-method.h"
#include "FunctionUtils.h"
#include "HierarchyUtils.h"
#include "TypeUtils.h"

#include <clang/AST/DeclCXX.h>
#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/Type.h>
#include <clang/Basic/LLVM.h>
#include <llvm/Support/Casting.h>

#include <vector>

using namespace clang;

SkippedBaseMethod::SkippedBaseMethod(const std::string &name, ClazyContext *context)
    : CheckBase(name, context)
{
}

void SkippedBaseMethod::VisitStmt(clang::Stmt *stmt)
{
    auto *memberCall = dyn_cast<CXXMemberCallExpr>(stmt);
    if (!memberCall) {
        return;
    }

    auto *expr = memberCall->getImplicitObjectArgument();
    auto *thisExpr = clazy::unpeal<CXXThisExpr>(expr, clazy::IgnoreImplicitCasts);
    if (!thisExpr) {
        return;
    }

    const CXXRecordDecl *thisClass = thisExpr->getType()->getPointeeCXXRecordDecl();
    const CXXRecordDecl *baseClass = memberCall->getRecordDecl();

    std::vector<CXXRecordDecl *> baseClasses;
    if (!clazy::derivesFrom(thisClass, baseClass, &baseClasses) || baseClasses.size() < 2) {
        return;
    }

    // We're calling a grand-base method, so check if a more direct base also implements it
    for (int i = baseClasses.size() - 1; i > 0; --i) { // the higher indexes have the most derived classes
        CXXRecordDecl *moreDirectBaseClass = baseClasses[i];
        if (clazy::classImplementsMethod(moreDirectBaseClass, memberCall->getMethodDecl())) {
            std::string msg =
                "Maybe you meant to call " + moreDirectBaseClass->getNameAsString() + "::" + memberCall->getMethodDecl()->getNameAsString() + "() instead";
            emitWarning(stmt, msg);
        }
    }
}
