/*
    SPDX-FileCopyrightText: 2016 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLAZY_INCORRECT_EMIT_H
#define CLAZY_INCORRECT_EMIT_H

#include "checkbase.h"

#include <clang/Basic/SourceLocation.h>

#include <string>
#include <unordered_map>
#include <vector>

namespace clang
{
class CXXMemberCallExpr;
class MacroInfo;
class Token;
}

/**
 * See README-incorrect-emit.md for more info.
 */
class IncorrectEmit : public CheckBase
{
public:
    explicit IncorrectEmit(const std::string &name, ClazyContext *context);
    void VisitStmt(clang::Stmt *stmt) override;

private:
    void checkCallSignalInsideCTOR(clang::CXXMemberCallExpr *);
    void VisitMacroExpands(const clang::Token &MacroNameTok, const clang::SourceRange &range, const clang::MacroInfo *minfo = nullptr) override;
    bool hasEmitKeyboard(clang::CXXMemberCallExpr *) const;
    std::vector<clang::SourceLocation> m_emitLocations;
    mutable std::unordered_map<unsigned, clang::SourceLocation> m_locationCache;
};

#endif
