/*
    SPDX-FileCopyrightText: 2016 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "returning-void-expression.h"
#include "ClazyContext.h"
#include "ContextUtils.h"
#include "clazy_stl.h"

#include <clang/AST/Decl.h>
#include <clang/AST/Expr.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/Type.h>
#include <clang/Basic/LLVM.h>
#include <llvm/Support/Casting.h>

using namespace clang;

void ReturningVoidExpression::VisitStmt(clang::Stmt *stmt)
{
    auto *ret = dyn_cast<ReturnStmt>(stmt);
    if (!ret || !clazy::hasChildren(ret)) {
        return;
    }

    QualType qt = ret->getRetValue()->getType();
    if (qt.isNull() || !qt->isVoidType()) {
        return;
    }

    DeclContext *context = clazy::contextForDecl(m_context->lastDecl);
    if (!context) {
        return;
    }

    auto *func = dyn_cast<FunctionDecl>(context);
    // A function template returning T won't bailout in the void check above, do it properly now:
    if (!func || !func->getReturnType()->isVoidType()) {
        return;
    }

    emitWarning(stmt, "Returning a void expression");
}
