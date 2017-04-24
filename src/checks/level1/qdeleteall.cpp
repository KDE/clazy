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

static bool isInterestingMethod(const string &name)
{
    static const vector<string> names = { "values", "keys" };
    return clazy_std::contains(names, name);
}

void QDeleteAll::VisitStmt(clang::Stmt *stmt)
{
    // Find a call to QMap/QSet/QHash::values/keys
    CXXMemberCallExpr *offendingCall = dyn_cast<CXXMemberCallExpr>(stmt);
    FunctionDecl *func = offendingCall ? offendingCall->getDirectCallee() : nullptr;
    if (!func)
        return;

    const string funcName = func->getNameAsString();
    if (isInterestingMethod(funcName)) {
        const std::string offendingClassName = offendingCall->getMethodDecl()->getParent()->getNameAsString();
        if (QtUtils::isQtAssociativeContainer(offendingClassName)) {
            // Once found see if the first parent call is qDeleteAll
            int i = 1;
            Stmt *p = HierarchyUtils::parent(m_parentMap, stmt, i);
            while (p) {
                CallExpr *pc = dyn_cast<CallExpr>(p);
                FunctionDecl *f = pc ? pc->getDirectCallee() : nullptr;
                if (f) {
                    if (f->getNameAsString() == "qDeleteAll") {
                        string msg = "Calling qDeleteAll with " + offendingClassName + "::" + funcName;
                        if (func->getNumParams() == 0) {
                            msg += ", call qDeleteAll on the container itself";
                        }

                        emitWarning(p->getLocStart(), msg);
                    }
                    break;
                }
                ++i;
                p = HierarchyUtils::parent(m_parentMap, stmt, i);
            }
        }
    }
}

REGISTER_CHECK_WITH_FLAGS("qdeleteall", QDeleteAll, CheckLevel1)
