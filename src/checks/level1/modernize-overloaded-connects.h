/*
    SPDX-FileCopyrightText: 2026 Alexander Lohnau <alexander.lohnau@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLAZY_MODERNIZE_OVERLOADED_SIGNALS_H
#define CLAZY_MODERNIZE_OVERLOADED_SIGNALS_H

#include "checkbase.h"

/**
 * See README-modernize-overloaded-signals.md for more info.
 */
class ModernizeOverloadedConnects : public CheckBase
{
public:
    using CheckBase::CheckBase;
    void VisitStmt(clang::Stmt *) override;

private:
    void checkConnectArg(clang::CallExpr *call, int numArgToCheck);
};

#endif
