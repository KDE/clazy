/*
    Copyright (C) 2025 Author <your@email>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLAZY_MUTEX_DETACHING_H
#define CLAZY_MUTEX_DETACHING_H

#include "checkbase.h"

/**
 * See README-mutex-detaching.md for more info.
 */
class ReadlockDetaching : public CheckBase
{
public:
    explicit ReadlockDetaching(const std::string &name, ClazyContext *context);
    void VisitDecl(clang::Decl *) override;
    void VisitStmt(clang::Stmt *) override;
    void registerASTMatchers(clang::ast_matchers::MatchFinder &) override;

private:
    ClazyAstMatcherCallback *const m_astMatcherCallBack;
};

#endif
