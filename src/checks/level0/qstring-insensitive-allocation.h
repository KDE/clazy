/*
    SPDX-FileCopyrightText: 2016 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLAZY_QSTRING_INSENSITIVE_ALLOCATION_H
#define CLAZY_QSTRING_INSENSITIVE_ALLOCATION_H

#include "checkbase.h"

#include <string>

/**
 * Finds unneeded allocations in the form of str.{toLower, toUpper}().{contains, compare, startsWith, endsWith}().
 *
 * See README-qstring-insensitive-allocation for more information
 */
class QStringInsensitiveAllocation : public CheckBase
{
public:
    explicit QStringInsensitiveAllocation(const std::string &name);
    void VisitStmt(clang::Stmt *stmt) override;
};

#endif
