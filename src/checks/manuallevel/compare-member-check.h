/*
    SPDX-FileCopyrightText: 2023 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.com>
    SPDX-FileContributor: Shivam Kunwar <shivam.kunwar@kdab.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLAZY_COMPARE_MEMBER_CHECK_H
#define CLAZY_COMPARE_MEMBER_CHECK_H

#include "checkbase.h"

#include <string>
#include <vector>

class Caller;
class ClazyContext;

namespace clang
{
namespace ast_matchers
{
class MatchFinder;
} // namespace ast_matchers
class Stmt;
class VarDecl;
class CXXRecordDecl;
class QualType;
} // namespace clang

class CompareMemberCheck : public CheckBase
{
public:
    using CheckBase::CheckBase;
    ~CompareMemberCheck();
    void VisitStmt(clang::Stmt *stmt) override;
    void registerASTMatchers(clang::ast_matchers::MatchFinder &) override;

private:
    std::unique_ptr<ClazyAstMatcherCallback> m_astMatcherCallBack; // TODO: add std::propagate_const
};

#endif
