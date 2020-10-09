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

#ifndef CLAZY_QT6_QLATINSTRINGCHAR_TO_U_H
#define CLAZY_QT6_QLATINSTRINGCHAR_TO_U_H

#include "checkbase.h"

#include <vector>
#include <string>

class ClazyContext;

namespace clang {
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
class Qt6QLatin1StringCharToU
    : public CheckBase
{
public:
    explicit Qt6QLatin1StringCharToU(const std::string &name, ClazyContext *context);
    void VisitStmt(clang::Stmt *stmt) override;
    void VisitMacroExpands(const clang::Token &MacroNameTok,
                           const clang::SourceRange &range, const clang::MacroInfo *minfo = nullptr) override;
private:
    std::vector<clang::SourceLocation> m_listingMacroExpand;
    bool checkCTorExpr(clang::Stmt *stmt, bool check_parents);
    void lookForLeftOver(clang::Stmt *stmt, bool keep_looking = true);
    std::string buildReplacement(clang::Stmt *stmt, bool &noFix, bool extra = false, bool ancestorIsCondition = false,
                                 int ancestorConditionChildNumber = 0);

};

#endif
