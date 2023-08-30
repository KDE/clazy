/*
  This file is part of the clazy static checker.

    SPDX-FileCopyrightText: 2017 Klarälvdalens Datakonsult AB a KDAB Group company info@kdab.com
    Author: Sérgio Martins <sergio.martins@kdab.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLAZY_CONNECT_NOT_NORMALIZED_H
#define CLAZY_CONNECT_NOT_NORMALIZED_H

#include "checkbase.h"

#include <string>

class ClazyContext;

namespace clang
{
class CXXConstructExpr;
class CallExpr;
class Expr;
class Stmt;
}

/**
 * See README-connect-not-normalized.md for more info.
 */
class ConnectNotNormalized : public CheckBase
{
public:
    explicit ConnectNotNormalized(const std::string &name, ClazyContext *context);
    void VisitStmt(clang::Stmt *stmt) override;

private:
    bool handleQ_ARG(clang::CXXConstructExpr *);
    bool handleConnect(clang::CallExpr *);
};

#endif
