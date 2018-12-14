/*
  This file is part of the clazy static checker.

    Copyright (C) Sérgio Martins <iamsergio@gmail.com>

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

#ifndef CLAZY_BODYRANGE_H
#define CLAZY_BODYRANGE_H

#include <clang/AST/Stmt.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/SourceManager.h>

struct StmtBodyRange
{
    clang::Stmt *body = nullptr;
    const clang::SourceManager *const sm = nullptr;
    const clang::SourceLocation searchUntilLoc; // We don't search after this point

    explicit StmtBodyRange(clang::Stmt *body,
                           const clang::SourceManager *sm = nullptr,
                           clang::SourceLocation searchUntilLoc = {})
        : body(body)
        , sm(sm)
        , searchUntilLoc(searchUntilLoc)
    {
    }

    bool isValid() const
    {
        return body != nullptr;
    }

    bool isOutsideRange(clang::Stmt *stmt) const
    {
        return isOutsideRange(stmt ? clazy::getLocStart(stmt) : clang::SourceLocation());
    }

    bool isOutsideRange(clang::SourceLocation loc) const
    {
        if (loc.isInvalid())
            return true;

        if (!sm || searchUntilLoc.isInvalid())
            return false;

        return sm->isBeforeInSLocAddrSpace(searchUntilLoc, loc);
    }
};

#endif
