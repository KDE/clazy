/*
    This file is part of the clazy static checker.

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

#ifndef CLAZY_IS_EMPTY_VS_COUNT_H
#define CLAZY_IS_EMPTY_VS_COUNT_H

#include "checkbase.h"

#include <string>

class ClazyContext;

namespace clang {
class Stmt;
}

/**
 * Finds places where you're using Container::count() instead of Container::isEmpty()
 *
 * See README-isempty-vs-count
 */
class IsEmptyVSCount
    : public CheckBase
{
public:
    explicit IsEmptyVSCount(const std::string &name, ClazyContext *context);
    void VisitStmt(clang::Stmt *stmt) override;
};

#endif
