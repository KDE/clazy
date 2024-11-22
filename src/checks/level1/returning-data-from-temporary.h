/*
    SPDX-FileCopyrightText: 2016-2017 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLAZY_RETURNING_DATA_FROM_TEMPORARY_H
#define CLAZY_RETURNING_DATA_FROM_TEMPORARY_H

#include "checkbase.h"

#include <string>

namespace clang
{
class CXXMemberCallExpr;
class DeclStmt;
class ReturnStmt;
}

/**
 * See README-returning-data-from-temporary.md for more info.
 */
class ReturningDataFromTemporary : public CheckBase
{
public:
    explicit ReturningDataFromTemporary(const std::string &name, ClazyContext *context);
    void VisitStmt(clang::Stmt *stmt) override;

private:
    bool handleReturn(clang::ReturnStmt *);
    void handleDeclStmt(clang::DeclStmt *);
    void handleMemberCall(clang::CXXMemberCallExpr *, bool onlyTemporaries);
};

#endif
