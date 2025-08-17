/*
    SPDX-FileCopyrightText: 2018 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "qstring-varargs.h"
#include "StringUtils.h"

#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/Expr.h>
#include <clang/AST/OperationKinds.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/Type.h>
#include <clang/Basic/LLVM.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Casting.h>

using namespace clang;

void QStringVarargs::VisitStmt(clang::Stmt *stmt)
{
    auto *binop = dyn_cast<BinaryOperator>(stmt);
    if (!binop || binop->getOpcode() != BO_Comma) {
        return;
    }

    auto *callexpr = dyn_cast<CallExpr>(binop->getLHS());
    if (!callexpr) {
        return;
    }

    const FunctionDecl *func = callexpr->getDirectCallee();
    if (!func || clazy::name(func) != "__builtin_trap") {
        return;
    }

    QualType qt = binop->getRHS()->getType();
    const CXXRecordDecl *record = qt->getAsCXXRecordDecl();
    if (!record) {
        return;
    }

    StringRef name = clazy::name(record);
    if (name == "QString" || name == "QByteArray") {
        emitWarning(stmt, std::string("Passing ") + name.data() + std::string(" to variadic function"));
    }
}
