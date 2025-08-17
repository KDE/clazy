/*
    SPDX-FileCopyrightText: 2018 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "raw-environment-function.h"
#include "StringUtils.h"

#include <clang/AST/Decl.h>
#include <clang/AST/Expr.h>
#include <clang/AST/Stmt.h>
#include <clang/Basic/LLVM.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Casting.h>

using namespace clang;

void RawEnvironmentFunction::VisitStmt(clang::Stmt *stmt)
{
    auto *callexpr = dyn_cast<CallExpr>(stmt);
    if (!callexpr) {
        return;
    }

    const FunctionDecl *func = callexpr->getDirectCallee();
    if (!func) {
        return;
    }

    StringRef funcName = clazy::name(func);

    if (funcName == "putenv") {
        emitWarning(stmt, "Prefer using qputenv instead of putenv");
    }

    if (funcName == "getenv") {
        emitWarning(stmt, "Prefer using qgetenv instead of getenv");
    }
}
