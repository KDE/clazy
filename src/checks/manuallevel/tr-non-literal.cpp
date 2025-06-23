/*
    SPDX-FileCopyrightText: 2017 Klarälvdalens Datakonsult AB a KDAB Group company info@kdab.com
    SPDX-FileContributor: Sérgio Martins <sergio.martins@kdab.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "tr-non-literal.h"
#include "HierarchyUtils.h"

#include <clang/AST/Decl.h>
#include <clang/AST/Expr.h>
#include <clang/AST/Stmt.h>
#include <clang/Basic/LLVM.h>
#include <llvm/Support/Casting.h>

using namespace clang;

TrNonLiteral::TrNonLiteral(const std::string &name)
    : CheckBase(name, Option_CanIgnoreIncludes)
{
}

void TrNonLiteral::VisitStmt(clang::Stmt *stmt)
{
    auto *callExpr = dyn_cast<CallExpr>(stmt);
    if (!callExpr || callExpr->getNumArgs() <= 0) {
        return;
    }

    FunctionDecl *func = callExpr->getDirectCallee();
    if (!func || func->getQualifiedNameAsString() != "QObject::tr") {
        return;
    }

    Expr *arg1 = callExpr->getArg(0);
    if (clazy::getFirstChildOfType2<StringLiteral>(arg1) == nullptr) {
        emitWarning(stmt, "tr() without a literal string");
    }
}
