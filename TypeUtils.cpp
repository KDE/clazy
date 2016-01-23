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

#include "TypeUtils.h"
#include <clang/Frontend/CompilerInstance.h>
#include <clang/AST/ASTContext.h>

using namespace clang;

int TypeUtils::sizeOfPointer(const clang::CompilerInstance &ci, const clang::QualType &qt)
{
    if (!qt.getTypePtrOrNull())
        return -1;
    // HACK: What's a better way of getting the size of a pointer ?
    auto &astContext = ci.getASTContext();
    return astContext.getTypeSize(astContext.getPointerType(qt));
}
