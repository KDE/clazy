/*
    SPDX-FileCopyrightText: 2016 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "qstring-left.h"
#include "StringUtils.h"

#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/Stmt.h>
#include <clang/Basic/LLVM.h>
#include <llvm/ADT/APInt.h>
#include <llvm/Support/Casting.h>

using namespace clang;

QStringLeft::QStringLeft(const std::string &name)
    : CheckBase(name, Option_CanIgnoreIncludes)
{
}

void QStringLeft::VisitStmt(clang::Stmt *stmt)
{
    auto *memberCall = dyn_cast<CXXMemberCallExpr>(stmt);
    if (!memberCall || clazy::qualifiedMethodName(memberCall) != "QString::left") {
        return;
    }

    if (memberCall->getNumArgs() == 0) { // Doesn't happen
        return;
    }

    Expr *firstArg = memberCall->getArg(0);
    auto *lt = firstArg ? dyn_cast<IntegerLiteral>(firstArg) : nullptr;
    if (lt) {
        const auto value = lt->getValue();
        if (value == 0) {
            emitWarning(stmt, "QString::left(0) returns an empty string");
        } else if (value == 1) {
            emitWarning(stmt, "Use QString::at(0) instead of QString::left(1) to avoid temporary allocations (just be sure the string isn't empty).");
        }
    }
}
