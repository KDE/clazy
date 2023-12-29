/*
    SPDX-FileCopyrightText: 2019 Sergio Martins <smartins@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "heap-allocated-small-trivial-type.h"
#include "ClazyContext.h"
#include "HierarchyUtils.h"
#include "QtUtils.h"
#include "StmtBodyRange.h"
#include "TypeUtils.h"
#include "Utils.h"

#include <clang/AST/AST.h>

using namespace clang;

HeapAllocatedSmallTrivialType::HeapAllocatedSmallTrivialType(const std::string &name, ClazyContext *context)
    : CheckBase(name, context)
{
}

void HeapAllocatedSmallTrivialType::VisitDecl(clang::Decl *decl)
{
    auto *varDecl = dyn_cast<VarDecl>(decl);
    if (!varDecl) {
        return;
    }

    Expr *init = varDecl->getInit();
    if (!init) {
        return;
    }

    auto *newExpr = dyn_cast<CXXNewExpr>(init);
    if (!newExpr || newExpr->getNumPlacementArgs() > 0) { // Placement new, user probably knows what he's doing
        return;
    }

    if (newExpr->isArray()) {
        return;
    }

    DeclContext *context = varDecl->getDeclContext();
    FunctionDecl *fDecl = context ? dyn_cast<FunctionDecl>(context) : nullptr;
    if (!fDecl) {
        return;
    }

    QualType qualType = newExpr->getType()->getPointeeType();
    if (clazy::isSmallTrivial(m_context, qualType)) {
        if (clazy::contains(qualType.getAsString(), "Private")) {
            // Possibly a pimpl, forward declared in header
            return;
        }

        auto *body = fDecl->getBody();
        if (Utils::isAssignedTo(body, varDecl) || Utils::isPassedToFunction(StmtBodyRange(body), varDecl, false) || Utils::isReturned(body, varDecl)) {
            return;
        }

        emitWarning(init, "Don't heap-allocate small trivially copyable/destructible types: " + qualType.getAsString(lo()));
    }
}
