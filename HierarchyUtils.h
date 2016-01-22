/*
   This file is part of the clang-lazy static checker.

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

#ifndef CLAZY_HIERARCHY_UTILS_H
#define CLAZY_HIERARCHY_UTILS_H

// Contains utility classes to retrieve parents and childs from AST Nodes

#include <clang/AST/Stmt.h>
#include <clang/AST/ExprCXX.h>

namespace clang {
class ParentMap;
}

namespace HierarchyUtils {

template <typename T>
T* getFirstChildOfType(clang::Stmt *stm)
{
    if (!stm)
        return nullptr;

    for (auto it = stm->child_begin(), end = stm->child_end(); it != end; ++it) {
        if (!*it) // Can happen
            continue;

        if (auto s = clang::dyn_cast<T>(*it))
            return s;

        if (auto s = getFirstChildOfType<T>(*it))
            return s;
    }

    return nullptr;
}

// Like getFirstChildOfType() but only looks at first child, so basically first branch of the tree
template <typename T>
T* getFirstChildOfType2(clang::Stmt *stm)
{
    if (!stm)
        return nullptr;

    if (stm->child_begin() != stm->child_end()) {
        auto child = *(stm->child_begin());
        if (auto s = clang::dyn_cast<T>(child))
            return s;

        if (auto s = getFirstChildOfType<T>(child))
            return s;
    }

    return nullptr;
}

// If depth = 0, return s
// If depth = 1, returns parent of s
// etc.
clang::Stmt* parent(clang::ParentMap *, clang::Stmt *s, unsigned int depth = 1);

// Returns the first parent of type T, with max depth depth
template <typename T>
T* getFirstParentOfType(clang::ParentMap *pmap, clang::Stmt *s, unsigned int depth = -1)
{
    if (!s)
        return nullptr;

    if (auto t = clang::dyn_cast<T>(s))
        return t;

    if (depth == 0)
        return nullptr;

    --depth;
    return getFirstParentOfType<T>(pmap, parent(pmap, s), depth);
}

clang::Stmt * getFirstChildAtDepth(clang::Stmt *parent, unsigned int depth);

std::vector<clang::Stmt*> childs(clang::Stmt *parent);

/// Goes into a statement and returns it's childs of type T
/// It only goes down 1 level of children, except if there's a ExprWithCleanups, which we unpeal

template <typename T>
void getChilds(clang::Stmt *stm, std::vector<T*> &result_list)
{
    if (!stm)
        return;

    auto expr = clang::dyn_cast<T>(stm);
    if (expr) {
        result_list.push_back(expr);
        return;
    }

    for (auto it = stm->child_begin(), e = stm->child_end(); it != e; ++it) {
        if (*it == nullptr) // Can happen
            continue;

        auto expr = clang::dyn_cast<T>(*it);
        if (expr) {
            result_list.push_back(expr);
            continue;
        }

        auto cleanups = clang::dyn_cast<clang::ExprWithCleanups>(*it);
        if (cleanups) {
            getChilds<T>(cleanups, result_list);
        }
    }
}

template <typename T>
void getChilds2(clang::Stmt *stmt, std::vector<T*> &result_list, int depth = -1)
{
    if (!stmt)
        return;

    auto cexpr = llvm::dyn_cast<T>(stmt);
    if (cexpr)
        result_list.push_back(cexpr);

    auto it = stmt->child_begin();
    auto end = stmt->child_end();

    if (depth > 0 || depth == -1) {
        if (depth > 0)
            --depth;
        for (; it != end; ++it) {
            getChilds2(*it, result_list, depth);
        }
    }
}

}

#endif
