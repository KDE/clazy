/*
    SPDX-FileCopyrightText: 2018 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLAZY_QSTRING_VARARGS_H
#define CLAZY_QSTRING_VARARGS_H

#include "checkbase.h"

#include <string>

/**
 * See README-qstring-varargs.md for more info.
 */
class QStringVarargs : public CheckBase
{
public:
    explicit QStringVarargs(const std::string &name);
    void VisitStmt(clang::Stmt *stmt) override;

private:
};

#endif
