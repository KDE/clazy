/*
    This file is part of the clazy static checker.

    Copyright (C) 2023 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
    Author: Shivam Kunwar <shivam.kunwar@kdab.com>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef CLAZY_UNUSED_RESULT_CHECK_H
#define CLAZY_UNUSED_RESULT_CHECK_H

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

class UnusedResultCheck : public CheckBase
{
public:
    explicit UnusedResultCheck(const std::string &name, ClazyContext *context);
    ~UnusedResultCheck();
    void VisitStmt(clang::Stmt *stmt) override;
    void registerASTMatchers(clang::ast_matchers::MatchFinder &) override;

private:
    std::unique_ptr<ClazyAstMatcherCallback> m_astMatcherCallBack; // TODO: add std::propagate_const
};

#endif
