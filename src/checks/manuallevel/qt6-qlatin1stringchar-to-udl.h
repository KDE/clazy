/*
    SPDX-FileCopyrightText: 2020 The Qt Company Ltd.
    SPDX-FileCopyrightText: 2020 Lucie Gerard <lucie.gerard@qt.io>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLAZY_QT6_QLATINSTRINGCHAR_TO_U_H
#define CLAZY_QT6_QLATINSTRINGCHAR_TO_U_H

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
 * Replaces QLatin1String( ) calls with u""
 * Replaces QLatin1Char( ) calls with u''.
 *
 */
class Qt6QLatin1StringCharToUdl : public CheckBase
{
public:
    explicit Qt6QLatin1StringCharToUdl(const std::string &name, ClazyContext *context);
    void VisitStmt(clang::Stmt *stmt) override;
    void VisitMacroExpands(const clang::Token &MacroNameTok, const clang::SourceRange &range, const clang::MacroInfo *minfo = nullptr) override;

private:
    std::vector<clang::SourceLocation> m_listingMacroExpand;
    std::vector<clang::SourceLocation> m_emittedWarningsInMacro;
    bool warningAlreadyEmitted(clang::SourceLocation sploc);
    bool checkCTorExpr(clang::Stmt *stmt, bool check_parents);
    void lookForLeftOver(clang::Stmt *stmt);
    std::string buildReplacement(clang::Stmt *stmt, bool &noFix, bool extra = false, bool ancestorIsCondition = false, int ancestorConditionChildNumber = 0);

    std::optional<std::string> isInterestingCtorCall(clang::CXXConstructExpr *ctorExpr, const ClazyContext *const context, bool check_parent = true);

    void insertUsingNamespace(clang::Stmt *stmt);

    bool m_has_using_namespace = false;
};

#endif
