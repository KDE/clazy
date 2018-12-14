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

#include "qstring-left.h"
#include "StringUtils.h"

#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/Stmt.h>
#include <clang/Basic/LLVM.h>
#include <llvm/ADT/APInt.h>
#include <llvm/Support/Casting.h>

class ClazyContext;

using namespace clang;
using namespace std;


QStringLeft::QStringLeft(const std::string &name, ClazyContext *context)
    : CheckBase(name, context, Option_CanIgnoreIncludes)
{
}

void QStringLeft::VisitStmt(clang::Stmt *stmt)
{
    auto memberCall = dyn_cast<CXXMemberCallExpr>(stmt);
    if (!memberCall || clazy::qualifiedMethodName(memberCall) != "QString::left")
        return;

    if (memberCall->getNumArgs() == 0) // Doesn't happen
        return;

    Expr *firstArg = memberCall->getArg(0);
    auto lt = firstArg ? dyn_cast<IntegerLiteral>(firstArg) : nullptr;
    if (lt) {
        const auto value = lt->getValue();
        if (value == 0) {
            emitWarning(stmt, "QString::left(0) returns an empty string");
        } else if (value == 1) {
            emitWarning(stmt, "Use QString::at(0) instead of QString::left(1) to avoid temporary allocations (just be sure the string isn't empty).");
        }
    }
}
