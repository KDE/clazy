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

#include "clazy_stl.h"

#include <clang/AST/ASTContext.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Lex/Lexer.h>
#include <clang/Lex/PreprocessorOptions.h>
#include <clang/Basic/SourceLocation.h>

#include <vector>

namespace clang {
class CompilerInstance;
class SourceLocation;
}

namespace clazy
{

/**
 * Returns true is macroName was defined via compiler invocation argument.
 * Like $ gcc -Dfoo main.cpp
 */
inline bool isPredefined(const clang::PreprocessorOptions &ppOpts, const llvm::StringRef &macroName)
{
    const auto &macros = ppOpts.Macros;

    for (const auto &macro : macros) {
        if (macro.first == macroName)
            return true;
    }

    return false;
}

/**
 * Returns true if the source location loc is inside a macro named macroName.
 */
inline bool isInMacro(const clang::ASTContext *context, clang::SourceLocation loc, const llvm::StringRef &macroName)
{
    if (loc.isValid() && loc.isMacroID()) {
        llvm::StringRef macro = clang::Lexer::getImmediateMacroName(loc, context->getSourceManager(), context->getLangOpts());
        return macro == macroName;
    }

    return false;
}

/**
 * Returns true if the source location loc is inside any of the specified macros.
 */
inline bool isInAnyMacro(const clang::ASTContext *context, clang::SourceLocation loc, const std::vector<llvm::StringRef> &macroNames)
{
    return clazy::any_of(macroNames, [context, loc](const llvm::StringRef &macroName) {
            return isInMacro(context, loc, macroName);
        });
}

}

#endif
