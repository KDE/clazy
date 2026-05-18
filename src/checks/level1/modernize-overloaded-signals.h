/*
    Copyright (C) 2026 Author <your@email>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLAZY_MODERNIZE_OVERLOADED_SIGNALS_H
#define CLAZY_MODERNIZE_OVERLOADED_SIGNALS_H

#include "checkbase.h"

/**
 * See README-modernize-overloaded-signals.md for more info.
 */
class ModernizeOverloadedSignals : public CheckBase
{
public:
    explicit ModernizeOverloadedSignals(const std::string &name);
    void VisitDecl(clang::Decl *) override;
    void VisitStmt(clang::Stmt *) override;

private:
};

#endif
