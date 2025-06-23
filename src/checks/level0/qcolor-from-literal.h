/*
    SPDX-FileCopyrightText: 2017 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLAZY_QCOLOR_FROM_LITERAL_H
#define CLAZY_QCOLOR_FROM_LITERAL_H

#include "checkbase.h"

#include <string>

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
    explicit QColorFromLiteral(const std::string &name);
    ~QColorFromLiteral() override;
    void VisitStmt(clang::Stmt *stmt) override;
    void registerASTMatchers(clang::ast_matchers::MatchFinder &) override;

private:
    ClazyAstMatcherCallback *const m_astMatcherCallBack;
};

#endif
