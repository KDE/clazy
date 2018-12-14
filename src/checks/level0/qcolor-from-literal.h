/*
  This file is part of the clazy static checker.

    Copyright (C) 2017 Sergio Martins <smartins@kde.org>

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

#ifndef CLAZY_QCOLOR_FROM_LITERAL_H
#define CLAZY_QCOLOR_FROM_LITERAL_H

#include "checkbase.h"

#include <string>

class QColorFromLiteral_Callback;
class ClazyContext;
namespace clang {
class Stmt;
namespace ast_matchers {
class MatchFinder;
}  // namespace ast_matchers
}  // namespace clang

/**
 * See README-qcolor-from-literal.md for more info.
 */
class QColorFromLiteral
    : public CheckBase
{
public:
    explicit QColorFromLiteral(const std::string &name, ClazyContext *context);
    ~QColorFromLiteral();
    void VisitStmt(clang::Stmt *stmt) override;
    void registerASTMatchers(clang::ast_matchers::MatchFinder &) override;
private:
    ClazyAstMatcherCallback *const m_astMatcherCallBack;
};

#endif
