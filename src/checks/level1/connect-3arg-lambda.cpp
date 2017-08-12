/*
  This file is part of the clazy static checker.

  Copyright (C) 2017 Sergio Martins <sergio.martins@kdab.com>

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

#include "connect-3arg-lambda.h"
#include "Utils.h"
#include "HierarchyUtils.h"
#include "QtUtils.h"
#include "TypeUtils.h"
#include "checkmanager.h"

#include <clang/AST/AST.h>

using namespace clang;
using namespace std;


Connect3argLambda::Connect3argLambda(const std::string &name, ClazyContext *context)
    : CheckBase(name, context)
{
}


void Connect3argLambda::VisitStmt(clang::Stmt *stmt)
{
    auto callExpr = dyn_cast<CallExpr>(stmt);
    if (!callExpr)
        return;

    FunctionDecl *fdecl = callExpr->getDirectCallee();
    if (!QtUtils::isConnect(fdecl) || fdecl->getNumParams() != 3)
        return;

    auto lambda = HierarchyUtils::getFirstChildOfType2<LambdaExpr>(callExpr->getArg(2));
    if (!lambda)
        return;
    callExpr->getArg(0)->dump();

    // The sender can be: this
    auto senderThis = HierarchyUtils::unpeal<CXXThisExpr>(callExpr->getArg(0), HierarchyUtils::IgnoreImplicitCasts);

    // Or it can be: a declref (variable)
    DeclRefExpr *senderDeclRef = senderThis ? nullptr : HierarchyUtils::getFirstChildOfType2<DeclRefExpr>(callExpr->getArg(0));


    // If this is referenced inside the lambda body, for example, by calling a member function
    auto thisExprs = HierarchyUtils::getStatements<CXXThisExpr>(lambda->getBody());
    const bool lambdaHasThis = !thisExprs.empty();

    // The variables used inside the lambda
    auto declrefs = HierarchyUtils::getStatements<DeclRefExpr>(lambda->getBody());

    // If lambda doesn't do anything interesting, don't warn
    if (declrefs.empty() && !lambdaHasThis)
        return;

    if (lambdaHasThis && !senderThis && QtUtils::isQObject(thisExprs[0]->getType())) {
        emitWarning(stmt->getLocStart(), "Pass 'this' as the 3rd connect parameter");
        return;
    }

    ValueDecl *senderDecl = senderDeclRef ? senderDeclRef->getDecl() : nullptr;
    // We'll only warn if the lambda is dereferencing another QObject (besides the sender).
    bool found = false;
    for (auto declref : declrefs) {
        ValueDecl *decl = declref->getDecl();
        if (decl == senderDecl)
            continue; // It's the sender, continue.

        if (QtUtils::isQObject(decl->getType())) {
            found = true;
            break;
        }
    }

    if (found)
        emitWarning(stmt->getLocStart(), "Pass a context object as 3rd connect parameter");
}


REGISTER_CHECK("connect-3arg-lambda", Connect3argLambda, CheckLevel1)
