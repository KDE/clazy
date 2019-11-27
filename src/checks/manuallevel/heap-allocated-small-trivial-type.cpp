/*
  This file is part of the clazy static checker.

  Copyright (C) 2019 Sergio Martins <smartins@kde.org>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.
*/

#include "heap-allocated-small-trivial-type.h"
#include "Utils.h"
#include "StmtBodyRange.h"
#include "HierarchyUtils.h"
#include "QtUtils.h"
#include "TypeUtils.h"
#include "ClazyContext.h"

#include <clang/AST/AST.h>

using namespace clang;
using namespace std;


HeapAllocatedSmallTrivialType::HeapAllocatedSmallTrivialType(const std::string &name,
                                                             ClazyContext *context)
    : CheckBase(name, context)
{
}

void HeapAllocatedSmallTrivialType::VisitDecl(clang::Decl *decl)
{
    auto varDecl = dyn_cast<VarDecl>(decl);
    if (!varDecl)
        return;

    Expr *init = varDecl->getInit();
    if (!init)
        return;

    auto newExpr = dyn_cast<CXXNewExpr>(init);
    if (!newExpr || newExpr->getNumPlacementArgs() > 0) // Placement new, user probably knows what he's doing
        return;

    if (newExpr->isArray())
        return;

    DeclContext *context = varDecl->getDeclContext();
    FunctionDecl *fDecl = context ? dyn_cast<FunctionDecl>(context) : nullptr;
    if (!fDecl)
        return;

    QualType qualType = newExpr->getType()->getPointeeType();
    if (clazy::isSmallTrivial(m_context, qualType)) {
        if (clazy::contains(qualType.getAsString(), "Private")) {
            // Possibly a pimpl, forward declared in header
            return;
        }

        auto body = fDecl->getBody();
        if (Utils::isAssignedTo(body, varDecl) ||
            Utils::isPassedToFunction(StmtBodyRange(body), varDecl, false) ||
            Utils::isReturned(body, varDecl))
            return;

        emitWarning(init, "Don't heap-allocate small trivially copyable/destructible types: " + qualType.getAsString());
    }
}
