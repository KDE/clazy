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

#include "AccessSpecifierManager.h"
#include "PreProcessorVisitor.h"
#include "ClazyContext.h"

#include <clang/AST/ParentMap.h>

ClazyContext::ClazyContext(const clang::CompilerInstance &compiler)
    : ci(compiler)
    , astContext(ci.getASTContext())
    , m_noWerror(getenv("CLAZY_NO_WERROR") != nullptr) // Allows user to make clazy ignore -Werror
{
}

ClazyContext::~ClazyContext()
{
    delete preprocessorVisitor;
    delete accessSpecifierManager;
    delete parentMap;

    preprocessorVisitor = nullptr;
    accessSpecifierManager = nullptr;
    parentMap = nullptr;
}

void ClazyContext::enableAccessSpecifierManager()
{
    if (!accessSpecifierManager && !usingPreCompiledHeaders())
        accessSpecifierManager = new AccessSpecifierManager(ci);
}

void ClazyContext::enablePreprocessorVisitor()
{
    if (!preprocessorVisitor && !usingPreCompiledHeaders())
        preprocessorVisitor = new PreProcessorVisitor(ci);
}
