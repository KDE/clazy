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

#include "raw-environment-function.h"
#include "StringUtils.h"

#include <clang/AST/Decl.h>
#include <clang/AST/Expr.h>
#include <clang/AST/Stmt.h>
#include <clang/Basic/LLVM.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Casting.h>

class ClazyContext;

using namespace clang;
using namespace std;


RawEnvironmentFunction::RawEnvironmentFunction(const std::string &name, ClazyContext *context)
    : CheckBase(name, context)
{
}

void RawEnvironmentFunction::VisitStmt(clang::Stmt *stmt)
{
    auto callexpr = dyn_cast<CallExpr>(stmt);
    if (!callexpr)
        return;

    FunctionDecl *func = callexpr->getDirectCallee();
    if (!func)
        return;

    StringRef funcName = clazy::name(func);

    if (funcName == "putenv")
        emitWarning(stmt, "Prefer using qputenv instead of putenv");

    if (funcName == "getenv")
        emitWarning(stmt, "Prefer using qgetenv instead of getenv");
}
