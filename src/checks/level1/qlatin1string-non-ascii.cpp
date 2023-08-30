/*
  This file is part of the clazy static checker.

    SPDX-FileCopyrightText: 2017 Klarälvdalens Datakonsult AB a KDAB Group company info@kdab.com
    Author: Sérgio Martins <sergio.martins@kdab.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "qlatin1string-non-ascii.h"
#include "HierarchyUtils.h"
#include "Utils.h"

#include <clang/AST/DeclCXX.h>
#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/Stmt.h>
#include <clang/Basic/LLVM.h>
#include <llvm/Support/Casting.h>

class ClazyContext;

using namespace clang;

QLatin1StringNonAscii::QLatin1StringNonAscii(const std::string &name, ClazyContext *context)
    : CheckBase(name, context, Option_CanIgnoreIncludes)
{
}

void QLatin1StringNonAscii::VisitStmt(clang::Stmt *stmt)
{
    auto *constructExpr = dyn_cast<CXXConstructExpr>(stmt);
    CXXConstructorDecl *ctor = constructExpr ? constructExpr->getConstructor() : nullptr;

    if (!ctor || ctor->getQualifiedNameAsString() != "QLatin1String::QLatin1String") {
        return;
    }

    auto *lt = clazy::getFirstChildOfType2<StringLiteral>(stmt);
    if (lt && !Utils::isAscii(lt)) {
        emitWarning(stmt, "QLatin1String with non-ascii literal");
    }
}
