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

#include <clang/AST/AST.h>

using namespace clang;
using namespace std;

Connect3ArgLambda::Connect3ArgLambda(const std::string &name, ClazyContext *context)
    : CheckBase(name, context, Option_CanIgnoreIncludes)
{
}

void Connect3ArgLambda::VisitStmt(clang::Stmt *stmt)
{
    auto callExpr = dyn_cast<CallExpr>(stmt);
    if (!callExpr)
        return;

    FunctionDecl *fdecl = callExpr->getDirectCallee();
    if (!fdecl || fdecl->getNumParams() != 3 || !QtUtils::isConnect(fdecl))
        return;

    auto lambda = HierarchyUtils::getFirstChildOfType2<LambdaExpr>(callExpr->getArg(2));
    if (!lambda)
        return;

    DeclRefExpr *senderDeclRef = nullptr;
    MemberExpr *senderMemberExpr = nullptr;

    Stmt *s = callExpr->getArg(0);
    while (s) {
        if ((senderDeclRef = dyn_cast<DeclRefExpr>(s)))
            break;

        if ((senderMemberExpr = dyn_cast<MemberExpr>(s)))
            break;

        s = HierarchyUtils::getFirstChild(s);
    }


    // The sender can be: this
    CXXThisExpr* senderThis = HierarchyUtils::unpeal<CXXThisExpr>(callExpr->getArg(0), HierarchyUtils::IgnoreImplicitCasts);

    // The variables used inside the lambda
    auto declrefs = HierarchyUtils::getStatements<DeclRefExpr>(lambda->getBody());

    ValueDecl *senderDecl = senderDeclRef ? senderDeclRef->getDecl() : nullptr;

    // We'll only warn if the lambda is dereferencing another QObject (besides the sender)
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

    if (!found) {
        auto thisexprs = HierarchyUtils::getStatements<CXXThisExpr>(lambda->getBody());
        if (!thisexprs.empty() && !senderThis)
            found = true;
    }

    if (found)
        emitWarning(stmt->getLocStart(), "Pass a context object as 3rd connect parameter");
}
