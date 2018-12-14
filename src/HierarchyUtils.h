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

// Contains utility classes to retrieve parents and childs from AST Nodes

#include "clazy_stl.h"
#include "StringUtils.h"

#include <clang/Frontend/CompilerInstance.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/ParentMap.h>

namespace clazy {

enum IgnoreStmt {
    IgnoreNone             = 0,
    IgnoreImplicitCasts    = 1,
    IgnoreExprWithCleanups = 2
};

typedef int IgnoreStmts;

/**
 * Returns true if child is a child of parent.
 */
inline bool isChildOf(clang::Stmt *child, clang::Stmt *parent)
{
    if (!child || !parent)
        return false;

    return clazy::any_of(parent->children(), [child](clang::Stmt *c) {
            return c == child || isChildOf(child, c);
        });
}

/**
 * Returns true if stm is parent of a member function call named "name"
 */

inline bool isParentOfMemberFunctionCall(clang::Stmt *stm, const std::string &name)
{
    if (!stm)
        return false;

    if (auto expr = llvm::dyn_cast<clang::MemberExpr>(stm)) {
        auto namedDecl = llvm::dyn_cast<clang::NamedDecl>(expr->getMemberDecl());
        if (namedDecl && clazy::name(namedDecl) == name)
            return true;
    }

    return clazy::any_of(stm->children(), [name] (clang::Stmt *child) {
            return isParentOfMemberFunctionCall(child, name);
        });

    return false;
}

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

    if (clazy::hasChildren(stm)) {
        auto child = *(stm->child_begin());

        if (!child) // can happen
            return nullptr;

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
inline clang::Stmt *parent(clang::ParentMap *map, clang::Stmt *s, unsigned int depth = 1)
{
    if (!s)
        return nullptr;

    return depth == 0 ? s
                      : clazy::parent(map, map->getParent(s), depth - 1);
}

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

inline clang::Stmt *getFirstChild(clang::Stmt *parent)
{
    if (!parent)
        return nullptr;

    auto it = parent->child_begin();
    return it == parent->child_end() ? nullptr : *it;
}

inline clang::Stmt * getFirstChildAtDepth(clang::Stmt *s, unsigned int depth)
{
    if (depth == 0 || !s)
        return s;

    return clazy::hasChildren(s) ? getFirstChildAtDepth(*s->child_begin(), --depth)
                                 : nullptr;
}

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

inline bool isIgnoredByOption(clang::Stmt *s, IgnoreStmts options)
{
    return ((options & IgnoreImplicitCasts)    && llvm::isa<clang::ImplicitCastExpr>(s)) ||
           ((options & IgnoreExprWithCleanups) && llvm::isa<clang::ExprWithCleanups>(s));
}

/**
 * Returns all statements of type T in body, starting from startLocation, or from body->getLocStart() if
 * startLocation is null.
 *
 * Similar to getChilds(), but with startLocation support.
 */
template <typename T>
std::vector<T*> getStatements(clang::Stmt *body,
                              const clang::SourceManager *sm = nullptr,
                              clang::SourceLocation startLocation = {},
                              int depth = -1, bool includeParent = false,
                              IgnoreStmts ignoreOptions = IgnoreNone)
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
            if (!startLocation.isValid() || (sm && sm->isBeforeInSLocAddrSpace(sm->getSpellingLoc(startLocation), clazy::getLocStart(child))))
                statements.push_back(childT);
        }

        if (!isIgnoredByOption(child, ignoreOptions))
            --depth;

        auto childStatements = getStatements<T>(child, sm, startLocation, depth, false, ignoreOptions);
        clazy::append(childStatements, statements);
    }

    return statements;
}

/**
 * If stmt is of type T, then stmt is returned.
 * If stmt is of type IgnoreImplicitCast or IgnoreExprWithCleanups (depending on options) then stmt's
 * first child is tested instead (recurses).
 * Otherwise nullptr is returned.
 *
 * This is useful for example when the interesting statement is under an Implicit cast, so:
 **/
template <typename T>
T* unpeal(clang::Stmt *stmt, IgnoreStmts options = IgnoreNone)
{
    if (!stmt)
        return nullptr;

    if (auto tt = llvm::dyn_cast<T>(stmt))
        return tt;

    if ((options & IgnoreImplicitCasts) && llvm::isa<clang::ImplicitCastExpr>(stmt))
        return unpeal<T>(clazy::getFirstChild(stmt), options);

    if ((options & IgnoreExprWithCleanups) && llvm::isa<clang::ExprWithCleanups>(stmt))
        return unpeal<T>(clazy::getFirstChild(stmt), options);

    return nullptr;
}

inline clang::SwitchStmt* getSwitchFromCase(clang::ParentMap *pmap, clang::CaseStmt *caseStm)
{
    return getFirstParentOfType<clang::SwitchStmt>(pmap, caseStm);
}

}

#endif
