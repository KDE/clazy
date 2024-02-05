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
template<typename T>
inline clang::SourceLocation getLocStart(const T *t)
{
#if LLVM_VERSION_MAJOR >= 8
    return t->getBeginLoc();
#else
    return t->getLocStart();
#endif
}

template<typename T>
inline clang::SourceLocation getLocEnd(const T *t)
{
#if LLVM_VERSION_MAJOR >= 8
    return t->getEndLoc();
#else
    return t->getLocEnd();
#endif
}

inline bool hasUnusedResultAttr(clang::FunctionDecl *func)
{
    auto RetType = func->getReturnType();
    if (const auto *Ret = RetType->getAsRecordDecl()) {
        if (const auto *R = Ret->getAttr<clang::WarnUnusedResultAttr>()) {
            return R != nullptr;
        }
    } else if (const auto *ET = RetType->getAs<clang::EnumType>()) {
        if (const clang::EnumDecl *ED = ET->getDecl()) {
            if (const auto *R = ED->getAttr<clang::WarnUnusedResultAttr>()) {
                return R != nullptr;
            }
        }
    }
    return func->getAttr<clang::WarnUnusedResultAttr>() != nullptr;
}

inline clang::tooling::Replacements &DiagnosticFix(clang::tooling::Diagnostic &diag, llvm::StringRef filePath)
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
#define GET_LEXER(id, inputFile, sm, lo) clang::Lexer(id, inputFile.value(), sm, lo)
#elif LLVM_VERSION_MAJOR >= 12
#define GET_LEXER(id, inputFile, sm, lo) clang::Lexer(id, inputFile.getValue(), sm, lo)
#else
#define GET_LEXER(id, inputFile, sm, lo) clang::Lexer(id, inputFile, sm, lo)
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
using OptionalFileEntryRef = const clang::FileEntry *;
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
