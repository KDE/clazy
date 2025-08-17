/*
    SPDX-FileCopyrightText: 2017 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLAZY_QCOLOR_FROM_LITERAL_H
#define CLAZY_QCOLOR_FROM_LITERAL_H

#include "checkbase.h"

class QColorFromLiteral_Callback;
namespace clang
{
namespace ast_matchers
{
class MatchFinder;
} // namespace ast_matchers
} // namespace clang

/**
 * See README-qcolor-from-literal.md for more info.
 */
class QColorFromLiteral : public CheckBase
{
public:
    using CheckBase::CheckBase;
    ~QColorFromLiteral() override;
    void VisitStmt(clang::Stmt *stmt) override;
    void registerASTMatchers(clang::ast_matchers::MatchFinder &) override;

private:
    std::unique_ptr<ClazyAstMatcherCallback> m_astMatcherCallBack;
};

#endif
