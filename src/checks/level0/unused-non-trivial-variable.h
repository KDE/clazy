/*
    This file is part of the clazy static checker.

    SPDX-FileCopyrightText: 2016-2017 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLAZY_UNUSED_NON_TRIVIAL_VARIABLE_H
#define CLAZY_UNUSED_NON_TRIVIAL_VARIABLE_H

#include "checkbase.h"

#include <string>
#include <vector>

class ClazyContext;

namespace clang
{
class Stmt;
class VarDecl;
class CXXRecordDecl;
class QualType;
}

/**
 * Warns about unused Qt value classes.
 * Compilers usually only warn when trivial classes are unused and don't emit warnings for non-trivial classes.
 * This check has a whitelist of common Qt classes such as containers, QFont, QUrl, etc and warns for those too.
 *
 * See README-unused-non-trivial-variable.md for more information
 */
class UnusedNonTrivialVariable : public CheckBase
{
public:
    explicit UnusedNonTrivialVariable(const std::string &name, ClazyContext *context);
    void VisitStmt(clang::Stmt *stmt) override;

private:
    void handleVarDecl(clang::VarDecl *varDecl);
    bool isInterestingType(clang::QualType t) const;
    bool isUninterestingType(const clang::CXXRecordDecl *record) const;
    std::vector<std::string> m_userBlacklist;
    std::vector<std::string> m_userWhitelist;
};

#endif
