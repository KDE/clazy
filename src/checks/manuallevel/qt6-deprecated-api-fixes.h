/*
    SPDX-FileCopyrightText: 2020 The Qt Company Ltd.
    SPDX-FileCopyrightText: 2020 Lucie Gerard <lucie.gerard@qt.io>

    SPDX-License-Identifier: LGPL-2.0-or-later
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
    void fixForDeprecatedOperator(clang::Stmt *stmt, const std::string &className);
    std::string buildReplacementforQDir(clang::DeclRefExpr *decl_operator, bool isPointer, std::string replacement, const std::string &replacement_var2);
    std::string buildReplacementForQVariant(clang::DeclRefExpr *decl_operator, const std::string &replacement, const std::string &replacement_var2);
};

#endif
