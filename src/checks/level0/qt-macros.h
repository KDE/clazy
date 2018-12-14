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

#ifndef CLAZY_QT_MACROS_H
#define CLAZY_QT_MACROS_H

#include "checkbase.h"

#include <clang/Basic/SourceLocation.h>

#include <string>

class QtMacrosPreprocessorCallbacks;
class ClazyContext;
namespace clang {
class Token;
}  // namespace clang

/**
 * See README-qt-macros for more info.
 */
class QtMacros
    : public CheckBase
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
