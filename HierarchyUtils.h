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

#ifndef CLAZY_HIERARCHY_UTILS_H
#define CLAZY_HIERARCHY_UTILS_H
#include "clazylib_export.h"
// Contains utility classes to retrieve parents and childs from AST Nodes
#include "clazy_stl.h"
#include <clang/Frontend/CompilerInstance.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/ExprCXX.h>

namespace clang {
class ParentMap;
}

namespace HierarchyUtils {

/**
 * Returns true if child is a child of parent.
 */
CLAZYLIB_EXPORT bool isChildOf(clang::Stmt *child, clang::Stmt *parent);

/**
 * Returns true if stm is parent of a member function call named "name"
 */
CLAZYLIB_EXPORT bool isParentOfMemberFunctionCall(clang::Stmt *stm, const std::string &name);

/**
 * Returns the first child of stm of type T.
 * Does depth-first.
 */
template <typename T>
T* getFirstChildOfType(clang::Stmt *stm)
{
    if (!stm)
        return nullptr;

    for (auto child : stm->children()) {
        if (!child) // Can happen
            continue;

        if (auto s = clang::dyn_cast<T>(child))
            return s;

        if (auto s = getFirstChildOfType<T>(child))
            return s;
    }

    return nullptr;
}


/**
 * Returns the first child of stm of type T, but only looks at the first branch.
 */
template <typename T>
T* getFirstChildOfType2(clang::Stmt *stm)
{
    if (!stm)
        return nullptr;

    if (clazy_std::hasChildren(stm)) {
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
CLAZYLIB_EXPORT clang::Stmt* parent(clang::ParentMap *, clang::Stmt *s, unsigned int depth = 1);

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

CLAZYLIB_EXPORT clang::Stmt * getFirstChild(clang::Stmt *parent);

CLAZYLIB_EXPORT clang::Stmt * getFirstChildAtDepth(clang::Stmt *parent, unsigned int depth);

template <typename T>
void getChilds(clang::Stmt *stmt, std::vector<T*> &result_list, int depth = -1)
{
    if (!stmt)
        return;

    auto cexpr = llvm::dyn_cast<T>(stmt);
    if (cexpr)
        result_list.push_back(cexpr);

    if (depth > 0 || depth == -1) {
        if (depth > 0)
            --depth;
        for (auto child : stmt->children()) {
            getChilds(child, result_list, depth);
        }
    }
}
/**
 * Returns all statements of type T in body, starting from startLocation, or from body->getLocStart() if
 * startLocation is null.
 *
 * Similar to getChilds(), but with startLocation support.
 */
template <typename T>
std::vector<T*> getStatements(clang::Stmt *body, const clang::SourceManager *sm = nullptr, clang::SourceLocation startLocation = {}, int depth = -1, bool includeParent = false)
{
    std::vector<T*> statements;
    if (!body || depth == 0)
        return statements;

    if (includeParent)
        if (T *t = clang::dyn_cast<T>(body))
            statements.push_back(t);

    for (auto child : body->children()) {
        if (!child) continue; // can happen
        if (T *childT = clang::dyn_cast<T>(child)) {
            if (!startLocation.isValid() || (sm && sm->isBeforeInSLocAddrSpace(sm->getSpellingLoc(startLocation), child->getLocStart())))
                statements.push_back(childT);
        }

        auto childStatements = getStatements<T>(child, sm, startLocation, depth - 1);
        clazy_std::append(childStatements, statements);
    }

    return statements;
}

}

#endif
