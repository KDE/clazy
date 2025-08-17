/*
    SPDX-FileCopyrightText: 2023 Klarälvdalens Datakonsult AB a KDAB Group company info@kdab.com
    SPDX-FileContributor: Shivam Kunwar <shivam.kunwar@kdab.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLAZY_UNUSED_RESULT_CHECK_H
#define CLAZY_UNUSED_RESULT_CHECK_H

#include "checkbase.h"

#include <string>

namespace clang
{
namespace ast_matchers
{
class MatchFinder;
} // namespace ast_matchers
} // namespace clang

class UnusedResultCheck : public CheckBase
{
public:
    explicit UnusedResultCheck(const std::string &name);
    ~UnusedResultCheck() override;
    void registerASTMatchers(clang::ast_matchers::MatchFinder &) override;

private:
    std::unique_ptr<ClazyAstMatcherCallback> m_astMatcherCallBack;
};

#endif
