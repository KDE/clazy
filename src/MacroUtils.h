/*
    This file is part of the clazy static checker.

    SPDX-FileCopyrightText: 2016 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLAZY_MACRO_UTILS_H
#define CLAZY_MACRO_UTILS_H

#include "clazy_stl.h"

#include <clang/AST/ASTContext.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Lex/Lexer.h>
#include <clang/Lex/PreprocessorOptions.h>

#include <vector>

namespace clang
{
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
        if (macro.first == macroName) {
            return true;
        }
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
