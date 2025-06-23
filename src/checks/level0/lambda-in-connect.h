/*
    SPDX-FileCopyrightText: 2016-2017 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLAZY_LAMBDA_IN_CONNECT_H
#define CLAZY_LAMBDA_IN_CONNECT_H

#include "checkbase.h"

#include <string>

/**
 * <Description>
 *
 * See README-example-check for more information
 */
class LambdaInConnect : public CheckBase
{
public:
    explicit LambdaInConnect(const std::string &name);
    void VisitStmt(clang::Stmt *stmt) override;
};

#endif
