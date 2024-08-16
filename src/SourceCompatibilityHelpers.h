/*
    SPDX-FileCopyrightText: 2018 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef SOURCE_COMPAT_HELPERS
#define SOURCE_COMPAT_HELPERS

#include <clang/AST/Attr.h>
#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/Expr.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Frontend/FrontendDiagnostic.h>
#include <clang/Lex/Lexer.h>
#include <clang/Tooling/Core/Diagnostic.h>

namespace clazy
{
inline auto getBuffer(const clang::SourceManager &sm, clang::FileID id, bool *invalid)
{
    auto buffer = sm.getBufferOrNone(id);
#if LLVM_VERSION_MAJOR >= 15
    *invalid = !buffer.has_value();
#else
    *invalid = !buffer.hasValue();
#endif
    return buffer;
}

#define GET_LEXER(id, inputFile, sm, lo) clang::Lexer(id, inputFile.getValue(), sm, lo)

inline bool contains_lower(clang::StringRef haystack, clang::StringRef needle)
{
    return haystack.contains_insensitive(needle);
}

inline bool isAscii(clang::StringLiteral *lt)
{
#if LLVM_VERSION_MAJOR >= 15
    return lt->isOrdinary();
#else
    return lt->isAscii();
#endif
}

}

#endif
