/*
  This file is part of the clazy static checker.

  SPDX-FileCopyrightText: 2020 Sergio Martins <smartins@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLAZY_QSTRING_COMPARISON_TO_IMPLICIT_CHAR_H
#define CLAZY_QSTRING_COMPARISON_TO_IMPLICIT_CHAR_H

#include "checkbase.h"

/**
 * See README-qstring-comparison-to-implicit-char.md for more info.
 */
class QStringComparisonToImplicitChar : public CheckBase
{
public:
    explicit QStringComparisonToImplicitChar(const std::string &name, ClazyContext *context);
    void VisitStmt(clang::Stmt *) override;

private:
};

#endif
