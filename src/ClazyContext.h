/*
    This file is part of the clazy static checker.

    Copyright (C) 2017 Sergio Martins <smartins@kde.org>

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

#ifndef CLAZY_CONTEXT_H
#define CLAZY_CONTEXT_H

#include "SuppressionManager.h"
#include "clazy_stl.h"

#include <clang/Frontend/CompilerInstance.h>
#include <clang/Lex/PreprocessorOptions.h>
#include <clang/Basic/FileManager.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/SourceManager.h>
#include <llvm/Support/Regex.h>
#include <llvm/ADT/StringRef.h>

#include <string>
#include <vector>
#include <memory>
#include <utility>

// ClazyContext is just a struct to share data and code between all checks

namespace clang {
class CompilerInstance;
class ASTContext;
class ParentMap;
class SourceManager;
class FixItRewriter;
class CXXMethodDecl;
class Decl;
}

class AccessSpecifierManager;
class PreProcessorVisitor;
class FixItExporter;

class ClazyContext
{
public:

    enum ClazyOption {
        ClazyOption_None = 0,
        ClazyOption_NoFixitsInplace = 1,
        ClazyOption_NoFixitsAutoWrite = 2, // If enabled then fixits are reported, but not applied
        ClazyOption_AllFixitsEnabled = 4,
        ClazyOption_Qt4Compat = 8,
        ClazyOption_OnlyQt = 16, // Ignore non-Qt files. This is done by bailing out if QT_CORE_LIB is not set.
        ClazyOption_QtDeveloper = 32, // For running clazy on Qt itself, optional, but honours specific guidelines
        ClazyOption_VisitImplicitCode = 64, // Inspect compiler generated code aswell, useful for custom checks, if they need it
        ClazyOption_IgnoreIncludedFiles = 128 // Only warn for the current file being compiled, not on includes (useful for performance reasons)
    };
    typedef int ClazyOptions;

    explicit ClazyContext(const clang::CompilerInstance &ci,
                          const std::string &headerFilter,
                          const std::string &ignoreDirs,
                          ClazyOptions = ClazyOption_None);
    ~ClazyContext();

    bool usingPreCompiledHeaders() const
    {
        return !ci.getPreprocessorOpts().ImplicitPCHInclude.empty();
    }

    bool userDisabledWError() const
    {
        return m_noWerror;
    }

    bool fixitsAreInplace() const
    {
        return !(options & ClazyOption_NoFixitsInplace);
    }

    bool fixitsEnabled() const
    {
        return allFixitsEnabled || !requestedFixitName.empty();
    }

    bool isQtDeveloper() const
    {
        return options & ClazyOption_QtDeveloper;
    }

    bool ignoresIncludedFiles() const
    {
        return options & ClazyOption_IgnoreIncludedFiles;
    }

    bool isVisitImplicitCode() const
    {
        return options & ClazyContext::ClazyOption_VisitImplicitCode;
    }

    bool isOptionSet(const std::string &optionName) const
    {
        return clazy::contains(extraOptions, optionName);
    }

    bool fileMatchesLoc(const std::unique_ptr<llvm::Regex> &regex, clang::SourceLocation loc,  const clang::FileEntry **file) const
    {
        if (!regex)
            return false;

        if (!(*file)) {
            clang::FileID fid = sm.getDecomposedExpansionLoc(loc).first;
            *file = sm.getFileEntryForID(fid);
            if (!(*file)) {
                return false;
            }
        }

        llvm::StringRef fileName((*file)->getName());
        return regex->match(fileName);
    }

    bool shouldIgnoreFile(clang::SourceLocation loc) const
    {
        // 1. Process the regexp that excludes files
        const clang::FileEntry *file = nullptr;
        if (ignoreDirsRegex) {
            const bool matches = fileMatchesLoc(ignoreDirsRegex, loc, &file);
            if (matches)
                return true;
        }

        // 2. Process the regexp that includes files. Has lower priority.
        if (!headerFilterRegex || isMainFile(loc))
            return false;

        const bool matches = fileMatchesLoc(headerFilterRegex, loc, &file);
        if (!file)
            return false;

        return !matches;
    }

    bool isMainFile(clang::SourceLocation loc) const
    {
        if (loc.isMacroID())
            loc = sm.getExpansionLoc(loc);

        return sm.isInFileID(loc, sm.getMainFileID());
    }

    /**
     * We only enable it if a check needs it, for performance reasons
     */
    void enableAccessSpecifierManager();
    void enablePreprocessorVisitor();
    bool isQt() const;

    // TODO: More things will follow
    const clang::CompilerInstance &ci;
    clang::ASTContext &astContext;
    clang::SourceManager &sm;
    AccessSpecifierManager *accessSpecifierManager = nullptr;
    PreProcessorVisitor *preprocessorVisitor = nullptr;
    SuppressionManager suppressionManager;
    const bool m_noWerror;
    clang::ParentMap *parentMap = nullptr;
    const ClazyOptions options;
    const std::vector<std::string> extraOptions;
    clang::FixItRewriter *rewriter = nullptr;
    FixItExporter *exporter = nullptr;
    bool allFixitsEnabled = false;
    std::string requestedFixitName;
    clang::CXXMethodDecl *lastMethodDecl = nullptr;
    clang::FunctionDecl *lastFunctionDecl = nullptr;
    clang::Decl *lastDecl = nullptr;
    std::unique_ptr<llvm::Regex> headerFilterRegex;
    std::unique_ptr<llvm::Regex> ignoreDirsRegex;
};

#endif
