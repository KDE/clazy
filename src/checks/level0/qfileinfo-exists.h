/*
    SPDX-FileCopyrightText: 2016 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLAZY_QFILEINFO_EXISTS_H
#define CLAZY_QFILEINFO_EXISTS_H

#include "checkbase.h"

#include <string>

/**
 * Finds places using QFileInfo("foo").exists() instead of the faster version QFileInfo::exists("foo")
 *
 * See README-qfileinfo-exists for more information
 */
class QFileInfoExists : public CheckBase
{
public:
    explicit QFileInfoExists(const std::string &name);
    void VisitStmt(clang::Stmt *stmt) override;
};

#endif
