/*
    This file is part of the clazy static checker.

    SPDX-FileCopyrightText: 2015 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLAZY_WRONG_QGLOBALSTATIC_H
#define CLAZY_WRONG_QGLOBALSTATIC_H

#include "checkbase.h"

#include <string>

class ClazyContext;

namespace clang
{
class Stmt;
}

/**
 * Finds Q_QGLOBAL_STATICs being used with trivial classes.
 *
 * See README-wrong-qglobalstatic for more information
 */
class WrongQGlobalStatic : public CheckBase
{
public:
    explicit WrongQGlobalStatic(const std::string &name, ClazyContext *context);
    void VisitStmt(clang::Stmt *stmt) override;
};

#endif
