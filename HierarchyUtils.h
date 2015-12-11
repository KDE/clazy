/*
   This file is part of the clang-lazy static checker.

  Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
  Author: Sérgio Martins <sergio.martins@kdab.com>

  Copyright (C) 2015 Sergio Martins <smartins@kde.org>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

  As a special exception, permission is given to link this program
  with any edition of Qt, and distribute the resulting executable,
  without including the source code for Qt in the source distribution.
*/

#ifndef CLAZY_HIERARCHY_UTILS_H
#define CLAZY_HIERARCHY_UTILS_H

// Contains utility classes to retrieve parents and childs from AST Nodes

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

}

#endif
