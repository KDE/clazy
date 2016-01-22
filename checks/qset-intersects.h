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

#ifndef CLANG_QSET_INTERSECTS_H
#define CLANG_QSET_INTERSECTS_H

#include "checkbase.h"

namespace clang {
class Stmt;
}

/**
 * Finds cases where you're using set.intersect(other).isEmpty(), which is much slower than just
 * calling QSet::intersects()
 *
 * See README-qset-intersects for more information
 */
class QSetIntersects : public CheckBase
{
public:
    explicit QSetIntersects(const std::string &name, const clang::CompilerInstance &ci);
    void VisitStmt(clang::Stmt *stmt) override;
};

#endif
