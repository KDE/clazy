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
#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Frontend/FrontendDiagnostic.h>
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

}

#endif
