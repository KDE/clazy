/*
    SPDX-FileCopyrightText: 2018 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLAZY_IFNDEF_DEFINE_TYPO_H
#define CLAZY_IFNDEF_DEFINE_TYPO_H

#include "checkbase.h"

/**
 * See README-ifndef-define-typo.md for more info.
 */
class IfndefDefineTypo : public CheckBase
{
public:
    explicit IfndefDefineTypo(const std::string &name, ClazyContext *context);
    void VisitMacroDefined(const clang::Token &macroNameTok) override;
    void VisitDefined(const clang::Token &macroNameTok, const clang::SourceRange &) override;
    void VisitIfdef(clang::SourceLocation, const clang::Token &) override;
    void VisitIfndef(clang::SourceLocation, const clang::Token &) override;
    void VisitIf(clang::SourceLocation loc, clang::SourceRange conditionRange, clang::PPCallbacks::ConditionValueKind conditionValue) override;
    void VisitElif(clang::SourceLocation loc,
                   clang::SourceRange conditionRange,
                   clang::PPCallbacks::ConditionValueKind ConditionValue,
                   clang::SourceLocation ifLoc) override;
    void VisitElse(clang::SourceLocation loc, clang::SourceLocation ifLoc) override;
    void VisitEndif(clang::SourceLocation loc, clang::SourceLocation ifLoc) override;

    void maybeWarn(const std::string &define, clang::SourceLocation loc);

private:
    std::string m_lastIfndef;
};

#endif
