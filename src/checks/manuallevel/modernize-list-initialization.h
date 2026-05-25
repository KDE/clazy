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
    using CheckBase::CheckBase;
    void VisitStmt(clang::Stmt *) override;

private:
    void checkOperatorCallListInitialization(clang::SourceRange fixitSourceRange, clang::CXXOperatorCallExpr *operatorCall);
    std::vector<clang::CallExpr *> m_alreadyCheckedOperatorCalls;
};

#endif
