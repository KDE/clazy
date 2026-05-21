/*
    SPDX-FileCopyrightText: 2026 Author <your@email>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLAZY_MODERNIZE_LIST_INITIALIZATION_H
#define CLAZY_MODERNIZE_LIST_INITIALIZATION_H

#include "checkbase.h"

/**
 * See README-modernize-list-initialization.md for more info.
 */
class ModernizeListInitialization : public CheckBase
{
public:
    explicit ModernizeListInitialization(const std::string &name, Options options);
    void VisitDecl(clang::Decl *) override;
    void VisitStmt(clang::Stmt *) override;

private:
};

#endif
