/*
    SPDX-FileCopyrightText: 2018 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLAZY_QSTRING_VARARGS_H
#define CLAZY_QSTRING_VARARGS_H

#include "checkbase.h"

/**
 * See README-qstring-varargs.md for more info.
 */
class QStringVarargs : public CheckBase
{
public:
    using CheckBase::CheckBase;
    void VisitStmt(clang::Stmt *stmt) override;
};

#endif
