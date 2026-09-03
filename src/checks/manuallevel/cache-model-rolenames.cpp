/*
    SPDX-FileCopyrightText: 2026 Alexander Lohnau <alexander.lohnau@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "cache-model-rolenames.h"
#include "HierarchyUtils.h"
#include "QtUtils.h"
#include "TypeUtils.h"
#include "Utils.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/Expr.h"
#include "clang/AST/ExprCXX.h"
#include "clang/AST/Stmt.h"

#include <clang/AST/AST.h>
#include <vector>

using namespace clang;

void CacheModelRolenames::VisitDecl(clang::Decl *decl)
{
    auto declExpr = dyn_cast<CXXMethodDecl>(decl);
    if (!declExpr || declExpr->getNameAsString() != "roleNames") {
        return;
    }
    const auto *parent = dyn_cast<CXXRecordDecl>(declExpr->getParent());
    if (!clazy::derivesFrom(parent, qtNamespaced("QAbstractItemModel"))) {
        return;
    }

    std::vector<ReturnStmt *> stmts;
    clazy::getChilds<ReturnStmt>(declExpr->getBody(), stmts);
    if (stmts.size() != 1) {
        return;
    }

    const auto *returnExpr = stmts[0]->getRetValue();
    if (auto *cleanupExpr = dyn_cast<ExprWithCleanups>(returnExpr)) {
        if (isa<CXXConstructExpr>(cleanupExpr->getSubExpr()))
            emitWarning(cleanupExpr, "roleNames should be cached as static or member variable");
    }

    const auto *returnConstrExpr = dyn_cast<CXXConstructExpr>(returnExpr);
    if (!returnConstrExpr || returnConstrExpr->getNumArgs() != 1) {
        return;
    }

    const auto *argDecl = dyn_cast<DeclRefExpr>(returnConstrExpr->getArg(0)->IgnoreImplicit());
    if (!argDecl) {
        return;
    }

    if (auto *varDecl = dyn_cast<VarDecl>(argDecl->getDecl())) {
        if (varDecl->isStaticLocal() || varDecl->isStaticDataMember()) {
            return; // Cached as static variable is perfectly fine
        }
        if (varDecl->isCXXInstanceMember()) {
            return; // Cached as member variable is also fine
        }
        emitWarning(varDecl, "roleNames should be cached as static or member variable");
    }
}
