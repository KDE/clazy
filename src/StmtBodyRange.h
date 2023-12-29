/*
    Copyright (C) Sérgio Martins <iamsergio@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLAZY_BODYRANGE_H
#define CLAZY_BODYRANGE_H

#include <clang/AST/Stmt.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/SourceManager.h>

struct StmtBodyRange {
    clang::Stmt *body = nullptr;
    const clang::SourceManager *const sm = nullptr;
    const clang::SourceLocation searchUntilLoc; // We don't search after this point

    explicit StmtBodyRange(clang::Stmt *body, const clang::SourceManager *sm = nullptr, clang::SourceLocation searchUntilLoc = {})
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
        if (loc.isInvalid()) {
            return true;
        }

        if (!sm || searchUntilLoc.isInvalid()) {
            return false;
        }

        return sm->isBeforeInSLocAddrSpace(searchUntilLoc, loc);
    }
};

#endif
