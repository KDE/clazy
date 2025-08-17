/*
    SPDX-FileCopyrightText: 2017 Klarälvdalens Datakonsult AB a KDAB Group company info@kdab.com
    SPDX-FileContributor: Sérgio Martins <sergio.martins@kdab.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLAZY_CONNECT_NOT_NORMALIZED_H
#define CLAZY_CONNECT_NOT_NORMALIZED_H

#include "checkbase.h"

namespace clang
{
class CallExpr;
class Expr;
}

/**
 * See README-connect-not-normalized.md for more info.
 */
class ConnectNotNormalized : public CheckBase
{
public:
    using CheckBase::CheckBase;
    void VisitStmt(clang::Stmt *stmt) override;

private:
    bool handleQ_ARG(clang::Stmt *);
    bool handleConnect(clang::CallExpr *);
    bool checkNormalizedLiteral(clang::StringLiteral *, clang::Expr *);
};

#endif
