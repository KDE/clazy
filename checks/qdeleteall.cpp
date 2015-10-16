/*
   This file is part of the clang-lazy static checker.

  Copyright (C) 2015 Albert Astals Cid <albert.astals@canonical.com>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

  As a special exception, permission is given to link this program
  with any edition of Qt, and distribute the resulting executable,
  without including the source code for Qt in the source distribution.
*/

#include "qdeleteall.h"
#include "Utils.h"
#include "checkmanager.h"

#include <clang/AST/AST.h>

using namespace clang;

QDeleteAll::QDeleteAll(const std::string &name)
    : CheckBase(name)
{
}

void QDeleteAll::VisitStmt(clang::Stmt *stmt)
{
    // Find a call to QMap/QSet/QHash::values
    CXXMemberCallExpr *valuesCall = dyn_cast<CXXMemberCallExpr>(stmt);
    if (valuesCall &&
        valuesCall->getDirectCallee() &&
        valuesCall->getDirectCallee()->getNameAsString() == "values")
    {
        const std::string valuesClassName = valuesCall->getMethodDecl()->getParent()->getNameAsString();
        if (valuesClassName == "QMap" || valuesClassName == "QSet" || valuesClassName == "QHash") {
            // Once found see if the first parent call is qDeleteAll
            int i = 1;
            Stmt *p = Utils::parent(m_parentMap, stmt, i);
            while (p) {
                CallExpr *pc = dyn_cast<CallExpr>(p);
                if (pc) {
                    if (pc->getDirectCallee() && pc->getDirectCallee()->getNameAsString() == "qDeleteAll") {
                        emitWarning(p->getLocStart(), "Calling qDeleteAll with " + valuesClassName + "::values, call qDeleteAll on the container itself");
                    }
                    break;
                }
                ++i;
                p = Utils::parent(m_parentMap, stmt, i);
            }
        }
    }
}

REGISTER_CHECK("qdeleteall", QDeleteAll)
