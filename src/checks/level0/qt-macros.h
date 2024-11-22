/*
    SPDX-FileCopyrightText: 2016 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLAZY_QT_MACROS_H
#define CLAZY_QT_MACROS_H

#include "checkbase.h"

#include <clang/Basic/SourceLocation.h>

#include <string>

class QtMacrosPreprocessorCallbacks;

namespace clang
{
class Token;
} // namespace clang

/**
 * See README-qt-macros for more info.
 */
class QtMacros : public CheckBase
{
public:
    explicit QtMacros(const std::string &name, ClazyContext *context);

private:
    void checkIfDef(const clang::Token &MacroNameTok, clang::SourceLocation Loc);
    void VisitMacroDefined(const clang::Token &MacroNameTok) override;
    void VisitDefined(const clang::Token &macroNameTok, const clang::SourceRange &range) override;
    void VisitIfdef(clang::SourceLocation loc, const clang::Token &macroNameTok) override;

    bool m_OSMacroExists = false;
};

#endif
