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

#include "install-event-filter.h"
#include "Utils.h"
#include "HierarchyUtils.h"
#include "TypeUtils.h"

#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/Stmt.h>
#include <clang/Basic/LLVM.h>
#include <llvm/Support/Casting.h>

#include <vector>

class ClazyContext;

using namespace clang;
using namespace std;


InstallEventFilter::InstallEventFilter(const std::string &name, ClazyContext *context)
    : CheckBase(name, context, Option_CanIgnoreIncludes)
{
}

void InstallEventFilter::VisitStmt(clang::Stmt *stmt)
{
    auto memberCallExpr = dyn_cast<CXXMemberCallExpr>(stmt);
    if (!memberCallExpr || memberCallExpr->getNumArgs() != 1)
        return;

    FunctionDecl *func = memberCallExpr->getDirectCallee();
    if (!func || func->getQualifiedNameAsString() != "QObject::installEventFilter")
        return;

    Expr *expr = memberCallExpr->getImplicitObjectArgument();
    if (!expr)
        return;

    if (!isa<CXXThisExpr>(clazy::getFirstChildAtDepth(expr, 1)))
        return;

    Expr *arg1 = memberCallExpr->getArg(0);
    arg1 = arg1 ? arg1->IgnoreCasts() : nullptr;

    CXXRecordDecl *record = clazy::typeAsRecord(arg1);
    auto methods = Utils::methodsFromString(record, "eventFilter");

    for (auto method : methods) {
        if (method->getQualifiedNameAsString() != "QObject::eventFilter") // It overrides it, probably on purpose then, don't warn.
            return;
    }

    emitWarning(stmt, "'this' should usually be the filter object, not the monitored one.");
}
