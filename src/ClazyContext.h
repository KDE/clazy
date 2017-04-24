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

#include <clang/Frontend/CompilerInstance.h>
#include <clang/Lex/PreprocessorOptions.h>

// ClazyContext is just a struct to share data and code between all checks

namespace clang {
    class CompilerInstance;
    class ASTContext;
    class ParentMap;
}

class AccessSpecifierManager;
class PreProcessorVisitor;

class ClazyContext
{
public:
    explicit ClazyContext(const clang::CompilerInstance &ci);
    ~ClazyContext();

    bool usingPreCompiledHeaders() const
    {
        return !ci.getPreprocessorOpts().ImplicitPCHInclude.empty();
    }

    bool userDisabledWError() const
    {
        return m_noWerror;
    }

    /**
     * We only enable it if a check needs it, for performance reasons
     */
    void enableAccessSpecifierManager();
    void enablePreprocessorVisitor();

    // TODO: More things will follow
    const clang::CompilerInstance &ci;
    clang::ASTContext &astContext;
    AccessSpecifierManager *accessSpecifierManager = nullptr;
    PreProcessorVisitor *preprocessorVisitor = nullptr;
    SuppressionManager suppressionManager;
    const bool m_noWerror;
    clang::ParentMap *parentMap = nullptr;
};

#endif
