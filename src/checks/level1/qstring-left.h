/*
    SPDX-FileCopyrightText: 2016 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLAZY_QSTRING_LEFT_H
#define CLAZY_QSTRING_LEFT_H

#include "checkbase.h"

#include <string>

/**
 * See README-qstring-left for more info.
 */
class QStringLeft : public CheckBase
{
public:
    QStringLeft(const std::string &name, ClazyContext *context);
    void VisitStmt(clang::Stmt *stmt) override;

private:
};

#endif
