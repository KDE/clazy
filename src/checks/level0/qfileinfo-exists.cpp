/*
    SPDX-FileCopyrightText: 2016 Sergio Martins <smartins@kde.org>
    SPDX-FileCopyrightText: 2023 Alexander Lohnau <alexander.lohnau@gmx.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "qfileinfo-exists.h"
#include "HierarchyUtils.h"
#include "StringUtils.h"

#include <clang/AST/DeclCXX.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/Stmt.h>
#include <clang/Basic/LLVM.h>
#include <llvm/Support/Casting.h>

using namespace clang;

QFileInfoExists::QFileInfoExists(const std::string &name, ClazyContext *context)
    : CheckBase(name, context, Option_CanIgnoreIncludes)
{
}

void QFileInfoExists::VisitStmt(clang::Stmt *stmt)
{
    auto *existsCall = dyn_cast<CXXMemberCallExpr>(stmt);
    std::string methodName = clazy::qualifiedMethodName(existsCall);
    if (methodName != "QFileInfo::exists") {
        return;
    }

    auto *ctorExpr = clazy::getFirstChildOfType<CXXConstructExpr>(existsCall);
    if (!ctorExpr || clazy::simpleArgTypeName(ctorExpr->getConstructor(), 0, lo()) != "QString") {
        return;
    }

    std::string userArgText = Lexer::getSourceText(CharSourceRange::getTokenRange(ctorExpr->getArg(0)->getSourceRange()), sm(), lo()).str();
    emitWarning(stmt->getBeginLoc(),
                "Use the static QFileInfo::exists() instead. It's documented to be faster.",
                {clang::FixItHint::CreateReplacement(stmt->getSourceRange(), "QFileInfo::exists(" + userArgText + ")")});
}
