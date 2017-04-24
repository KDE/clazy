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

#include "child-event-qobject-cast.h"
#include "Utils.h"
#include "HierarchyUtils.h"
#include "QtUtils.h"
#include "TypeUtils.h"
#include "checkmanager.h"

#include <clang/AST/AST.h>
#include <clang/AST/DeclCXX.h>

using namespace clang;
using namespace std;


ChildEvent_qobject_cast::ChildEvent_qobject_cast(const std::string &name, ClazyContext *context)
    : CheckBase(name, context)
{
}

void ChildEvent_qobject_cast::VisitDecl(Decl *decl)
{
    auto childEventMethod = dyn_cast<CXXMethodDecl>(decl);
    if (!childEventMethod)
        return;

    Stmt *body = decl->getBody();
    if (!body)
        return;

    auto methodName = childEventMethod->getNameAsString();
    if (!clazy_std::equalsAny(methodName, {"event", "childEvent", "eventFilter"}))
        return;

    if (!QtUtils::isQObject(childEventMethod->getParent()))
        return;


    auto callExprs = HierarchyUtils::getStatements<CallExpr>(body, &(sm()));
    for (auto callExpr : callExprs) {

        if (callExpr->getNumArgs() != 1)
            continue;

        FunctionDecl *fdecl = callExpr->getDirectCallee();
        if (fdecl && fdecl->getNameAsString() == "qobject_cast")  {
            CXXMemberCallExpr *childCall = dyn_cast<CXXMemberCallExpr>(callExpr->getArg(0));
            // The call to event->child()
            if (!childCall)
                continue;

            auto childFDecl = childCall->getDirectCallee();
            if (!childFDecl || childFDecl->getQualifiedNameAsString() != "QChildEvent::child")
                continue;

            emitWarning(childCall, "qobject_cast in childEvent");
        }
    }
}



REGISTER_CHECK_WITH_FLAGS("child-event-qobject-cast", ChildEvent_qobject_cast, CheckLevel1)
