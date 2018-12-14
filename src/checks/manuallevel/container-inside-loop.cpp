/*
    This file is part of the clazy static checker.

    Copyright (C) 2016 Sergio Martins <smartins@kde.org>

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

#include "container-inside-loop.h"
#include "ClazyContext.h"
#include "Utils.h"
#include "StringUtils.h"
#include "LoopUtils.h"
#include "StmtBodyRange.h"
#include "SourceCompatibilityHelpers.h"
#include "clazy_stl.h"

#include <clang/AST/ParentMap.h>
#include <clang/AST/Decl.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/Stmt.h>
#include <clang/Basic/LLVM.h>
#include <llvm/Support/Casting.h>

namespace clang {
class CXXConstructorDecl;
}  // namespace clang

using namespace clang;
using namespace std;


ContainerInsideLoop::ContainerInsideLoop(const std::string &name, ClazyContext *context)
    : CheckBase(name, context, Option_CanIgnoreIncludes)
{
}

void ContainerInsideLoop::VisitStmt(clang::Stmt *stmt)
{
    auto *ctorExpr = dyn_cast<CXXConstructExpr>(stmt);
    if (!ctorExpr)
        return;

    CXXConstructorDecl *ctor = ctorExpr->getConstructor();
    if (!ctor || !clazy::equalsAny(clazy::classNameFor(ctor), { "QVector", "std::vector", "QList" }))
        return;

    DeclStmt *declStm = dyn_cast_or_null<DeclStmt>(m_context->parentMap->getParent(stmt));
    if (!declStm || !declStm->isSingleDecl())
        return;

    Stmt *loopStmt = clazy::isInLoop(m_context->parentMap, stmt);
    if (!loopStmt)
        return;

    auto *varDecl = dyn_cast<VarDecl>(declStm->getSingleDecl());
    if (!varDecl || Utils::isInitializedExternally(varDecl))
        return;

    if (Utils::isPassedToFunction(StmtBodyRange(loopStmt), varDecl, true))
        return;

    emitWarning(clazy::getLocStart(stmt), "container inside loop causes unneeded allocations");
}
