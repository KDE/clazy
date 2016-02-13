/*
   This file is part of the clazy static checker.

  Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
  Author: Sérgio Martins <sergio.martins@kdab.com>

  Copyright (C) 2015 Sergio Martins <smartins@kde.org>

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

#include "LoopUtils.h"
#include "clazy_stl.h"

#include <clang/AST/ParentMap.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/ExprCXX.h>

using namespace std;
using namespace clang;

Stmt *LoopUtils::bodyFromLoop(Stmt *loop)
{
    if (!loop)
        return nullptr;

    if (auto forstm = dyn_cast<ForStmt>(loop)) {
        return forstm->getBody();
    }

    if (auto rangeLoop = dyn_cast<CXXForRangeStmt>(loop)) {
        return rangeLoop->getBody();
    }

    if (auto whilestm = dyn_cast<WhileStmt>(loop)) {
        return whilestm->getBody();
    }

    if (auto dostm = dyn_cast<DoStmt>(loop)) {
        return dostm->getBody();
    }

    return nullptr;
}

bool LoopUtils::loopCanBeInterrupted(clang::Stmt *stmt, const clang::CompilerInstance &ci,
                                     const clang::SourceLocation &onlyBeforeThisLoc)
{
    if (!stmt)
        return false;

    if (isa<ReturnStmt>(stmt) || isa<BreakStmt>(stmt) || isa<ContinueStmt>(stmt)) {
        if (onlyBeforeThisLoc.isValid()) {
            FullSourceLoc sourceLoc(stmt->getLocStart(), ci.getSourceManager());
            FullSourceLoc otherSourceLoc(onlyBeforeThisLoc, ci.getSourceManager());
            if (sourceLoc.isBeforeInTranslationUnitThan(otherSourceLoc))
                return true;
        } else {
            return true;
        }
    }

    return clazy_std::any_of(stmt->children(), [&ci, onlyBeforeThisLoc](Stmt *s) {
        return LoopUtils::loopCanBeInterrupted(s, ci, onlyBeforeThisLoc);
    });
}

clang::Expr *LoopUtils::containerExprForLoop(Stmt *loop)
{
    if (!loop)
        return nullptr;

    if (auto rangeLoop = dyn_cast<CXXForRangeStmt>(loop)) {
        return rangeLoop->getRangeInit();
    }

    if (auto constructExpr = dyn_cast<CXXConstructExpr>(loop)) {
        if (constructExpr->getNumArgs() < 1)
            return nullptr;

        CXXConstructorDecl *constructorDecl = constructExpr->getConstructor();
        if (!constructorDecl || constructorDecl->getNameAsString() != "QForeachContainer")
            return nullptr;


        return constructExpr;
    }

    return nullptr;
}

Stmt* LoopUtils::isInLoop(clang::ParentMap *pmap, clang::Stmt *stmt)
{
    if (!stmt)
        return nullptr;

    Stmt *p = pmap->getParent(stmt);
    while (p) {
        if (LoopUtils::isLoop(p))
            return p;
        p = pmap->getParent(p);
    }

    return nullptr;
}
