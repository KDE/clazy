/*
    SPDX-FileCopyrightText: 2015 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLAZY_IS_EMPTY_VS_COUNT_H
#define CLAZY_IS_EMPTY_VS_COUNT_H

#include "checkbase.h"

#include <string>

class ClazyContext;

namespace clang
{
class Stmt;
}

/**
 * Finds places where you're using Container::count() instead of Container::isEmpty()
 *
 * See README-isempty-vs-count
 */
class IsEmptyVSCount : public CheckBase
{
public:
    explicit IsEmptyVSCount(const std::string &name, ClazyContext *context);
    void VisitStmt(clang::Stmt *stmt) override;
};

#endif
