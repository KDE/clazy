/*
    This file is part of the clazy static checker.

    SPDX-FileCopyrightText: 2016 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLAZY_CONTAINER_ANTI_PATTERN_H
#define CLAZY_CONTAINER_ANTI_PATTERN_H

#include "checkbase.h"

#include <string>

class ClazyContext;

namespace clang
{
class Stmt;
class CXXForRangeStmt;
class CXXConstructExpr;
}

/**
 * Warns when there are unneeded allocations of temporary lists because of using values(), keys()
 * toVector() or toList().
 *
 * See README-anti-pattern for more information
 */
class ContainerAntiPattern : public CheckBase
{
public:
    explicit ContainerAntiPattern(const std::string &name, ClazyContext *context);
    void VisitStmt(clang::Stmt *stmt) override;

private:
    bool VisitQSet(clang::Stmt *stmt);
    bool handleLoop(clang::Stmt *);
};

#endif
