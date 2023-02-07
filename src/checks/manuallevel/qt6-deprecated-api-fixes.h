/*
    This file is part of the clazy static checker.

    Copyright (C) 2020 The Qt Company Ltd.
    Copyright (C) 2020 Lucie Gerard <lucie.gerard@qt.io>

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

#ifndef CLAZY_QT6_DEPRECATEDAPI_FIXES
#define CLAZY_QT6_DEPRECATEDAPI_FIXES

#include "checkbase.h"

#include <string>
#include <vector>

class ClazyContext;

namespace clang
{
class Stmt;
class FixItHint;
class CXXConstructExpr;
class CXXOperatorCallExpr;
class Expr;
class CXXMemberCallExpr;

class CXXFunctionalCastExpr;
}

/**
 * Idenfify deprecated API and replace them when possible
 *
 * Run only with Qt 5.
 */
class Qt6DeprecatedAPIFixes : public CheckBase
{
public:
    explicit Qt6DeprecatedAPIFixes(const std::string &name, ClazyContext *context);
    void VisitStmt(clang::Stmt *stmt) override;
    void VisitDecl(clang::Decl *decl) override;
    void VisitMacroExpands(const clang::Token &MacroNameTok, const clang::SourceRange &range, const clang::MacroInfo *) override;

private:
    std::vector<clang::SourceLocation> m_listingMacroExpand;
    void fixForDeprecatedOperator(clang::Stmt *stmt, const std::string& className);
    std::string buildReplacementforQDir(clang::DeclRefExpr *decl_operator, bool isPointer, std::string replacement, const std::string& replacement_var2);
    std::string buildReplacementForQVariant(clang::DeclRefExpr *decl_operator, const std::string& replacement, const std::string& replacement_var2);
};

#endif
