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

#include "qstring-varargs.h"
#include "StringUtils.h"

#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/Expr.h>
#include <clang/AST/OperationKinds.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/Type.h>
#include <clang/Basic/LLVM.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Casting.h>

class ClazyContext;

using namespace clang;
using namespace std;


QStringVarargs::QStringVarargs(const std::string &name, ClazyContext *context)
    : CheckBase(name, context, Option_CanIgnoreIncludes)
{
}

void QStringVarargs::VisitStmt(clang::Stmt *stmt)
{
    auto binop = dyn_cast<BinaryOperator>(stmt);
    if (!binop || binop->getOpcode() != BO_Comma)
        return;

    auto callexpr = dyn_cast<CallExpr>(binop->getLHS());
    if (!callexpr)
        return;

    FunctionDecl *func = callexpr->getDirectCallee();
    if (!func || clazy::name(func) != "__builtin_trap")
        return;

    QualType qt = binop->getRHS()->getType();
    CXXRecordDecl *record = qt->getAsCXXRecordDecl();
    if (!record)
        return;

    StringRef name = clazy::name(record);
    if (name == "QString" || name == "QByteArray")
        emitWarning(stmt, string("Passing ") + name.data() + string(" to variadic function"));
}
