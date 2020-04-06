/*
  This file is part of the clazy static checker.

  Copyright (C) 2020 Sergio Martins <smartins@kde.org>

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

#ifndef CLAZY_KEEPING_UNSTABLE_REF_H
#define CLAZY_KEEPING_UNSTABLE_REF_H

#include "checkbase.h"


/**
 * See README-keeping-unstable-ref.md for more info.
 */
class KeepingUnstableRef : public CheckBase
{
public:
    explicit KeepingUnstableRef(const std::string &name, ClazyContext *context);
    void VisitStmt(clang::Stmt *) override;
private:
    bool processDeclRefCase(clang::CXXOperatorCallExpr *op);
    bool processMemberAssignment(clang::CXXOperatorCallExpr *op);
};

#endif
