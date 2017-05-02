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

#include <string>
#include <vector>

// ClazyContext is just a struct to share data and code between all checks

namespace clang {
    class CompilerInstance;
    class ASTContext;
    class ParentMap;
    class SourceManager;
    class FixItRewriter;
}

class AccessSpecifierManager;
class PreProcessorVisitor;

class ClazyContext
{
public:

    enum ClazyOption {
        ClazyOption_None = 0,
        ClazyOption_NoFixitsInplace = 1,
        ClazyOption_AllFixitsEnabled = 2,
        ClazyOption_Qt4Compat = 4
    };
    typedef int ClazyOptions;

    explicit ClazyContext(const clang::CompilerInstance &ci, ClazyOptions = ClazyOption_None);
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


    bool isOptionSet(const std::string &optionName) const
    {
        return clazy_std::contains(extraOptions, optionName);
    }

    /**
     * We only enable it if a check needs it, for performance reasons
     */
    void enableAccessSpecifierManager();
    void enablePreprocessorVisitor();

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
    clang::FixItRewriter *const rewriter;
    bool allFixitsEnabled = false;
    std::string requestedFixitName;
};

#endif
