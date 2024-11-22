/*
    SPDX-FileCopyrightText: 2016 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLAZY_RETURNING_VOID_EXPRESSION_H
#define CLAZY_RETURNING_VOID_EXPRESSION_H

#include "checkbase.h"

#include <string>

/**
 * See README-returning-void-expression.md for more info.
 */
class ReturningVoidExpression : public CheckBase
{
public:
    explicit ReturningVoidExpression(const std::string &name, ClazyContext *context);
    void VisitStmt(clang::Stmt *stmt) override;

private:
};

#endif
