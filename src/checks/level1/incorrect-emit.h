/*
  This file is part of the clazy static checker.

    Copyright (C) 2016 Sergio Martins <smartins@kde.org>

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

#ifndef CLAZY_INCORRECT_EMIT_H
#define CLAZY_INCORRECT_EMIT_H

#include "checkbase.h"

#include <clang/Basic/SourceLocation.h>

#include <unordered_map>
#include <string>
#include <vector>

class ClazyContext;

namespace clang {
class CXXMemberCallExpr;
class MacroInfo;
class Stmt;
class Token;
}

/**
 * See README-incorrect-emit.md for more info.
 */
class IncorrectEmit
    : public CheckBase
{
public:
    explicit IncorrectEmit(const std::string &name, ClazyContext *context);
    void VisitStmt(clang::Stmt *stmt) override;
private:
    void checkCallSignalInsideCTOR(clang::CXXMemberCallExpr *);
    void VisitMacroExpands(const clang::Token &MacroNameTok,
                           const clang::SourceRange &range, const clang::MacroInfo *minfo = nullptr) override;
    bool hasEmitKeyboard(clang::CXXMemberCallExpr *) const;
    std::vector<clang::SourceLocation> m_emitLocations;
    mutable std::unordered_map<unsigned, clang::SourceLocation> m_locationCache;
};

#endif
