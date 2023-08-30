/*
  This file is part of the clazy static checker.

    SPDX-FileCopyrightText: 2017 Klarälvdalens Datakonsult AB a KDAB Group company info@kdab.com
    Author: Sérgio Martins <sergio.martins@kdab.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLAZY_QLATIN1STRING_ENCODING_H
#define CLAZY_QLATIN1STRING_ENCODING_H

#include "checkbase.h"

#include <string>

class ClazyContext;
namespace clang
{
class Stmt;
} // namespace clang

/**
 * See README-qlatin1string-non-ascii.md for more info.
 */
class QLatin1StringNonAscii : public CheckBase
{
public:
    explicit QLatin1StringNonAscii(const std::string &name, ClazyContext *context);
    void VisitStmt(clang::Stmt *stmt) override;

private:
};

#endif
