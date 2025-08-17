/*
    SPDX-FileCopyrightText: 2016 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "container-inside-loop.h"
#include "ClazyContext.h"
#include "LoopUtils.h"
#include "StmtBodyRange.h"
#include "StringUtils.h"
#include "Utils.h"
#include "clazy_stl.h"

#include <clang/AST/Decl.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/ParentMap.h>
#include <clang/AST/Stmt.h>
#include <clang/Basic/LLVM.h>
#include <llvm/Support/Casting.h>

using namespace clang;

void ContainerInsideLoop::VisitStmt(clang::Stmt *stmt)
{
    auto *ctorExpr = dyn_cast<CXXConstructExpr>(stmt);
    if (!ctorExpr) {
        return;
    }

    CXXConstructorDecl *ctor = ctorExpr->getConstructor();
    if (!ctor || !clazy::equalsAny(clazy::classNameFor(ctor), {"QVector", "std::vector", "QList"})) {
        return;
    }

    auto *declStm = dyn_cast_or_null<DeclStmt>(m_context->parentMap->getParent(stmt));
    if (!declStm || !declStm->isSingleDecl()) {
        return;
    }

    Stmt *loopStmt = clazy::isInLoop(m_context->parentMap, stmt);
    if (!loopStmt) {
        return;
    }

    auto *varDecl = dyn_cast<VarDecl>(declStm->getSingleDecl());
    if (!varDecl || Utils::isInitializedExternally(varDecl)) {
        return;
    }

    if (Utils::isPassedToFunction(StmtBodyRange(loopStmt), varDecl, true)) {
        return;
    }

    emitWarning(stmt->getBeginLoc(), "container inside loop causes unneeded allocations");
}
