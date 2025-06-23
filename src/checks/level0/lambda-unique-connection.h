/*
    SPDX-FileCopyrightText: 2017 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLAZY_LAMBDA_UNIQUE_CONNECTION_H
#define CLAZY_LAMBDA_UNIQUE_CONNECTION_H

#include "checkbase.h"

#include <string>

/**
 * See README-lambda-unique-connection.md for more info.
 */
class LambdaUniqueConnection : public CheckBase
{
public:
    explicit LambdaUniqueConnection(const std::string &name);
    void VisitStmt(clang::Stmt *stmt) override;
};

#endif
