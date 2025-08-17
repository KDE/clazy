/*
    SPDX-FileCopyrightText: 2016 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLAZY_QFILEINFO_EXISTS_H
#define CLAZY_QFILEINFO_EXISTS_H

#include "checkbase.h"

/**
 * Finds places using QFileInfo("foo").exists() instead of the faster version QFileInfo::exists("foo")
 *
 * See README-qfileinfo-exists for more information
 */
class QFileInfoExists : public CheckBase
{
public:
    using CheckBase::CheckBase;
    void VisitStmt(clang::Stmt *stmt) override;
};

#endif
