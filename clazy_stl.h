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


#ifndef CLAZY_STL_H
#define CLAZY_STL_H

#include <clang/AST/Stmt.h>
#include <algorithm>

namespace clazy_std {

template<typename C>
bool contains(const C &container, const typename C::value_type &value)
{
   return std::find(container.cbegin(), container.cend(), value) != container.cend();
}

template<typename C, typename Pred>
bool contains_if(const C &container, Pred pred)
{
   return std::find_if(container.cbegin(), container.cend(), pred) != container.cend();
}

template<typename C>
typename C::iterator find(C &container, const typename C::value_type &value)
{
    return std::find(container.begin(), container.end(), value);
}

template<typename C>
typename C::const_iterator find(const C &container, const typename C::value_type &value)
{
    return std::find(container.cbegin(), container.cend(), value);
}

template<typename C, typename Pred>
typename C::iterator find_if(C &container, Pred pred)
{
    return std::find_if(container.begin(), container.end(), pred);
}

template<typename C, typename Pred>
typename C::const_iterator find_if(const C &container, Pred pred)
{
    return std::find_if(container.cbegin(), container.cend(), pred);
}

template<typename Pred>
bool any_child(clang::Stmt *s, Pred pred)
{
    return s ? std::any_of(s->child_begin(), s->child_end(), pred)
             : false;
}

}


#endif
