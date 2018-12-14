/*
  This file is part of the clazy static checker.

    Copyright (C) 2018 Sergio Martins <smartins@kde.org>

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

#ifndef CLAZY_IFNDEF_DEFINE_TYPO_H
#define CLAZY_IFNDEF_DEFINE_TYPO_H

#include "checkbase.h"

/**
 * See README-ifndef-define-typo.md for more info.
 */
class IfndefDefineTypo
    : public CheckBase
{
public:
    explicit IfndefDefineTypo(const std::string &name, ClazyContext *context);
    void VisitMacroDefined(const clang::Token &macroNameTok) override;
    void VisitDefined(const clang::Token &macroNameTok, const clang::SourceRange &) override;
    void VisitIfdef(clang::SourceLocation, const clang::Token &) override;
    void VisitIfndef(clang::SourceLocation, const clang::Token &) override;
    void VisitIf(clang::SourceLocation loc, clang::SourceRange conditionRange, clang::PPCallbacks::ConditionValueKind conditionValue) override;
    void VisitElif(clang::SourceLocation loc, clang::SourceRange conditionRange, clang::PPCallbacks::ConditionValueKind ConditionValue, clang::SourceLocation ifLoc) override;
    void VisitElse(clang::SourceLocation loc, clang::SourceLocation ifLoc) override;
    void VisitEndif(clang::SourceLocation loc, clang::SourceLocation ifLoc) override;

    void maybeWarn(const std::string &define, clang::SourceLocation loc);

private:
    std::string m_lastIfndef;
};

#endif
