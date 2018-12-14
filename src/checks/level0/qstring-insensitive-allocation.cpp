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

#include "qstring-insensitive-allocation.h"
#include "Utils.h"
#include "StringUtils.h"
#include "SourceCompatibilityHelpers.h"
#include "clazy_stl.h"

#include <clang/AST/Expr.h>
#include <clang/AST/Stmt.h>
#include <clang/Basic/LLVM.h>
#include <llvm/Support/Casting.h>

#include <vector>

class ClazyContext;
namespace clang {
class FunctionDecl;
}  // namespace clang

using namespace clang;
using namespace std;


QStringInsensitiveAllocation::QStringInsensitiveAllocation(const std::string &name, ClazyContext *context)
    : CheckBase(name, context, Option_CanIgnoreIncludes)
{
}

static bool isInterestingCall1(CallExpr *call)
{
    FunctionDecl *func = call->getDirectCallee();
    if (!func)
        return false;

    static const vector<string> methods = { "QString::toUpper", "QString::toLower" };
    return clazy::contains(methods, clazy::qualifiedMethodName(func));
}

static bool isInterestingCall2(CallExpr *call)
{
    FunctionDecl *func = call->getDirectCallee();
    if (!func)
        return false;

    static const vector<string> methods = { "QString::endsWith", "QString::startsWith",
                                            "QString::contains", "QString::compare" };
    return clazy::contains(methods, clazy::qualifiedMethodName(func));
}

void QStringInsensitiveAllocation::VisitStmt(clang::Stmt *stmt)
{
    vector<CallExpr *> calls = Utils::callListForChain(dyn_cast<CallExpr>(stmt));
    if (calls.size() < 2)
        return;

    CallExpr *call1 = calls[calls.size() - 1];
    CallExpr *call2 = calls[calls.size() - 2];

    if (!isInterestingCall1(call1) || !isInterestingCall2(call2))
        return;

    emitWarning(clazy::getLocStart(stmt), "unneeded allocation");
}
