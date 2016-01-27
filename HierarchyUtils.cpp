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

#include "HierarchyUtils.h"

#include <clang/AST/ParentMap.h>

using namespace std;
using namespace clang;

Stmt *HierarchyUtils::parent(ParentMap *map, Stmt *s, unsigned int depth)
{
    if (!s)
        return nullptr;

    return depth == 0 ? s
                      : parent(map, map->getParent(s), depth - 1);
}

clang::Stmt * HierarchyUtils::getFirstChildAtDepth(clang::Stmt *s, unsigned int depth)
{
    if (depth == 0 || !s)
        return s;

    return s->child_begin() == s->child_end() ? nullptr : getFirstChildAtDepth(*s->child_begin(), --depth);
}

vector<Stmt*> HierarchyUtils::childs(clang::Stmt *parent)
{
    vector<Stmt*> children;

    if (!parent)
        return children;

    for (auto it = parent->child_begin(), end = parent->child_end(); it != end; ++it)
        children.push_back(*it);

    return children;
}

bool HierarchyUtils::isChildOf(Stmt *child, Stmt *parent)
{
    if (!child || !parent)
        return false;

    for (auto c = parent->child_begin(), end = parent->child_end(); c != end; ++c) {
        if (*c == child || isChildOf(child, *c))
            return true;
    }

    return false;
}

bool HierarchyUtils::isParentOfMemberFunctionCall(Stmt *stm, const std::string &name)
{
    if (!stm)
        return false;

    auto expr = dyn_cast<MemberExpr>(stm);

    if (expr) {
        auto namedDecl = dyn_cast<NamedDecl>(expr->getMemberDecl());
        if (namedDecl && namedDecl->getNameAsString() == name)
            return true;
    }

    for (auto it = stm->child_begin(), e = stm->child_end(); it != e; ++it) {
        if (isParentOfMemberFunctionCall(*it, name))
            return true;
    }

    return false;
}
