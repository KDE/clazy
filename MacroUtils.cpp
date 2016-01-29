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

#include "MacroUtils.h"
#include "clazy_stl.h"
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Lex/Lexer.h>
#include <clang/Basic/SourceLocation.h>

using namespace std;
using namespace clang;

bool MacroUtils::isPredefined(const clang::CompilerInstance &ci, const string &macroName)
{
    const auto &macros = ci.getPreprocessorOpts().Macros;

    for (const auto &macro : macros) {
        if (macro.first == macroName)
            return true;
    }

    return false;
}


bool MacroUtils::isInMacro(const CompilerInstance &ci, SourceLocation loc, const string &macroName)
{
    if (loc.isValid() && loc.isMacroID()) {
        auto macro = Lexer::getImmediateMacroName(loc, ci.getSourceManager(), ci.getLangOpts());
        return macro == macroName;
    }

    return false;
}

bool MacroUtils::isInAnyMacro(const clang::CompilerInstance &ci, clang::SourceLocation loc, const std::vector<std::string> &macroNames)
{
    return clazy_std::any_of(macroNames, [&ci, loc](const string &macroName) {
        return isInMacro(ci, loc, macroName);
    });
}
