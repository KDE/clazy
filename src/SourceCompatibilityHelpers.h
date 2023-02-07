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


#ifndef SOURCE_COMPAT_HELPERS
#define SOURCE_COMPAT_HELPERS

#include <clang/AST/Attr.h>
#include <clang/AST/Expr.h>
#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Frontend/FrontendDiagnostic.h>
#include <clang/Lex/Lexer.h>
#include <clang/Tooling/Core/Diagnostic.h>

#if defined(CLAZY_USES_BOOST_REGEX)
# define BOOST_NO_EXCEPTIONS
# include <boost/throw_exception.hpp>
inline void boost::throw_exception(std::exception const &){}
# include <boost/regex.hpp>
using namespace boost;
#else
# include <regex>
using namespace std;
#endif

namespace clazy {

template <typename T>
inline clang::SourceLocation getLocStart(const T *t)
{
#if LLVM_VERSION_MAJOR >= 8
    return t->getBeginLoc();
#else
    return t->getLocStart();
#endif
}

template <typename T>
inline clang::SourceLocation getLocEnd(const T *t)
{
#if LLVM_VERSION_MAJOR >= 8
    return t->getEndLoc();
#else
    return t->getLocEnd();
#endif
}

inline clang::CharSourceRange getImmediateExpansionRange(clang::SourceLocation macroLoc, const clang::SourceManager &sm)
{
#if LLVM_VERSION_MAJOR >= 7
    return sm.getImmediateExpansionRange(macroLoc);
#else
    auto pair = sm.getImmediateExpansionRange(macroLoc);
    return clang::CharSourceRange(clang::SourceRange(pair.first, pair.second), false);
#endif
}

inline bool hasUnusedResultAttr(clang::FunctionDecl *func)
{
#if LLVM_VERSION_MAJOR >= 8
    auto RetType = func->getReturnType();
    if (const auto *Ret = RetType->getAsRecordDecl()) {
        if (const auto *R = Ret->getAttr<clang::WarnUnusedResultAttr>())
            return R != nullptr;
    } else if (const auto *ET = RetType->getAs<clang::EnumType>()) {
        if (const clang::EnumDecl *ED = ET->getDecl()) {
            if (const auto *R = ED->getAttr<clang::WarnUnusedResultAttr>())
                return R != nullptr;
        }
    }
    return func->getAttr<clang::WarnUnusedResultAttr>() != nullptr;
#else
    return func->hasUnusedResultAttr();
#endif

}

inline clang::tooling::Replacements& DiagnosticFix(clang::tooling::Diagnostic &diag, llvm::StringRef filePath)
{
#if LLVM_VERSION_MAJOR >= 9
    return diag.Message.Fix[filePath];
#else
    return diag.Fix[filePath];
#endif
}

inline auto getBuffer(const clang::SourceManager &sm, clang::FileID id, bool *invalid)
{
#if LLVM_VERSION_MAJOR >= 16
    auto buffer = sm.getBufferOrNone(id);
    *invalid = !buffer.has_value();
    return buffer;
#elif LLVM_VERSION_MAJOR >= 12
    auto buffer = sm.getBufferOrNone(id);
    *invalid = !buffer.hasValue();
    return buffer;
#else
    return sm.getBuffer(id, invalid);
#endif
}

#if LLVM_VERSION_MAJOR >= 16
#define GET_LEXER(id, inputFile, sm, lo) \
clang::Lexer(id, inputFile.value(), sm, lo)
#elif LLVM_VERSION_MAJOR >= 12
#define GET_LEXER(id, inputFile, sm, lo) \
clang::Lexer(id, inputFile.getValue(), sm, lo)
#else
#define GET_LEXER(id, inputFile, sm, lo) \
clang::Lexer(id, inputFile, sm, lo)
#endif

inline bool isFinal(const clang::CXXRecordDecl *record)
{
#if LLVM_VERSION_MAJOR >= 11
    return record->isEffectivelyFinal();
#else
    return record->hasAttr<clang::FinalAttr>();
#endif
}

inline bool contains_lower(clang::StringRef haystack, clang::StringRef needle)
{
#if LLVM_VERSION_MAJOR >= 13
    return haystack.contains_insensitive(needle);
#else
    return haystack.contains_lower(needle);
#endif
}

#if LLVM_VERSION_MAJOR >= 16
using OptionalFileEntryRef = clang::CustomizableOptional<clang::FileEntryRef>;
#elif LLVM_VERSION_MAJOR >= 15
using OptionalFileEntryRef = clang::Optional<clang::FileEntryRef>;
#else
using OptionalFileEntryRef = const clang::FileEntry*;
#endif

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
