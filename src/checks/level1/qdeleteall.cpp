/*
   This file is part of the clazy static checker.

  Copyright (C) 2015 Albert Astals Cid <albert.astals@canonical.com>

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

#include "qdeleteall.h"
#include "ClazyContext.h"
#include "Utils.h"
#include "HierarchyUtils.h"
#include "QtUtils.h"
#include "checkmanager.h"

#include <clang/AST/AST.h>
#include <vector>

using namespace clang;
using namespace std;

QDeleteAll::QDeleteAll(const std::string &name, ClazyContext *context)
    : CheckBase(name, context)
{
}

void QDeleteAll::VisitStmt(clang::Stmt *stmt)
{
    // Find a call to QMap/QSet/QHash::values/keys
    CXXMemberCallExpr *offendingCall = dyn_cast<CXXMemberCallExpr>(stmt);
    FunctionDecl *func = offendingCall ? offendingCall->getDirectCallee() : nullptr;
    if (!func)
        return;

    const string funcName = func->getNameAsString();
    const bool isValues = funcName == "values";
    const bool isKeys = isValues ? false : funcName == "keys";

    if (isValues || isKeys) {
        const std::string offendingClassName = offendingCall->getMethodDecl()->getParent()->getNameAsString();
        if (QtUtils::isQtAssociativeContainer(offendingClassName)) {
            // Once found see if the first parent call is qDeleteAll
            int i = 1;
            Stmt *p = HierarchyUtils::parent(m_context->parentMap, stmt, i);
            while (p) {
                CallExpr *pc = dyn_cast<CallExpr>(p);
                FunctionDecl *f = pc ? pc->getDirectCallee() : nullptr;
                if (f) {
                    if (f->getNameAsString() == "qDeleteAll") {
                        string msg = "qDeleteAll() is being used on an unnecessary temporary container created by " + offendingClassName + "::" + funcName + "()";
                        if (func->getNumParams() == 0) {
                            if (isValues) {
                                msg += ", use qDeleteAll(mycontainer) instead";
                            } else {
                                msg += ", use qDeleteAll(mycontainer.keyBegin(), mycontainer.keyEnd()) instead";
                            }
                        }

                        emitWarning(p->getLocStart(), msg);
                    }
                    break;
                }
                ++i;
                p = HierarchyUtils::parent(m_context->parentMap, stmt, i);
            }
        }
    }
}

REGISTER_CHECK("qdeleteall", QDeleteAll, CheckLevel1)
