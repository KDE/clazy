/*
    SPDX-FileCopyrightText: 2018 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLAZY_RAW_ENVIRONMENT_FUNCTION_H
#define CLAZY_RAW_ENVIRONMENT_FUNCTION_H

#include "checkbase.h"

#include <string>

/**
 * See README-raw-environment-function.md for more info.
 */
class RawEnvironmentFunction : public CheckBase
{
public:
    explicit RawEnvironmentFunction(const std::string &name);
    void VisitStmt(clang::Stmt *) override;

private:
};

#endif
