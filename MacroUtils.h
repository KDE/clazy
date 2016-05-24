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

#ifndef CLAZY_MACRO_UTILS_H
#define CLAZY_MACRO_UTILS_H

#include "clazylib_export.h"

#include <string>
#include <vector>

namespace clang {
class CompilerInstance;
class SourceLocation;
}

namespace MacroUtils
{

/**
 * Returns true is macroName was defined via compiler invocation argument.
 * Like $ gcc -Dfoo main.cpp
 */
CLAZYLIB_EXPORT bool isPredefined(const clang::CompilerInstance &, const std::string &macroName);

/**
 * Returns true if the source location loc is inside a macro named macroName.
 */
CLAZYLIB_EXPORT bool isInMacro(const clang::CompilerInstance &ci, clang::SourceLocation loc, const std::string &macroName);

/**
 * Returns true if the source location loc is inside any of the specified macros.
 */
CLAZYLIB_EXPORT bool isInAnyMacro(const clang::CompilerInstance &ci, clang::SourceLocation loc, const std::vector<std::string> &macroNames);

}

#endif
