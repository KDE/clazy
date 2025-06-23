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

namespace clang
{
class CXXConstructExpr;
}

/**
 * Replaces QLatin1String( ) calls with u""
 * Replaces QLatin1Char( ) calls with u''.
 *
 */
class Qt6QLatin1StringCharToU : public CheckBase
{
public:
    explicit Qt6QLatin1StringCharToU(const std::string &name);
    void VisitStmt(clang::Stmt *stmt) override;
    void VisitMacroExpands(const clang::Token &MacroNameTok, const clang::SourceRange &range, const clang::MacroInfo *minfo = nullptr) override;

private:
    std::vector<clang::SourceLocation> m_listingMacroExpand;
    std::vector<clang::SourceLocation> m_emittedWarningsInMacro;
    bool warningAlreadyEmitted(clang::SourceLocation sploc);
    bool checkCTorExpr(clang::Stmt *stmt, bool check_parents);
    void lookForLeftOver(clang::Stmt *stmt, bool found_QString_QChar = false);
    std::string buildReplacement(clang::Stmt *stmt, bool &noFix, bool extra = false, bool ancestorIsCondition = false, int ancestorConditionChildNumber = 0);

    bool isInterestingCtorCall(clang::CXXConstructExpr *ctorExpr, const ClazyContext *const context, bool check_parent = true);
    bool relatedToQStringOrQChar(clang::Stmt *stmt, const ClazyContext *const context);
    bool foundQCharOrQString(clang::Stmt *stmt);

    bool m_QStringOrQChar_fix = false;
    bool m_QChar = false;
    bool m_QChar_noFix = false;
};

#endif
