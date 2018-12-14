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

#include "qfileinfo-exists.h"
#include "StringUtils.h"
#include "HierarchyUtils.h"
#include "SourceCompatibilityHelpers.h"

#include <clang/AST/DeclCXX.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/Stmt.h>
#include <clang/Basic/LLVM.h>
#include <llvm/Support/Casting.h>

class ClazyContext;

using namespace clang;
using namespace std;


QFileInfoExists::QFileInfoExists(const std::string &name, ClazyContext *context)
    : CheckBase(name, context, Option_CanIgnoreIncludes)
{
}

void QFileInfoExists::VisitStmt(clang::Stmt *stmt)
{
    auto existsCall = dyn_cast<CXXMemberCallExpr>(stmt);
    std::string methodName = clazy::qualifiedMethodName(existsCall);
    if (methodName != "QFileInfo::exists")
        return;

    CXXConstructExpr* ctorExpr = clazy::getFirstChildOfType<CXXConstructExpr>(existsCall);
    if (!ctorExpr || clazy::simpleArgTypeName(ctorExpr->getConstructor(), 0, lo()) != "QString")
        return;

    emitWarning(clazy::getLocStart(stmt), "Use the static QFileInfo::exists() instead. It's documented to be faster.");
}
