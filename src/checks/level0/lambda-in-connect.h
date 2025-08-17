/*
    SPDX-FileCopyrightText: 2016-2017 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLAZY_LAMBDA_IN_CONNECT_H
#define CLAZY_LAMBDA_IN_CONNECT_H

#include "checkbase.h"

/**
 * See README-example-check for more information
 */
class LambdaInConnect : public CheckBase
{
public:
    using CheckBase::CheckBase;
    void VisitStmt(clang::Stmt *stmt) override;
};

#endif
