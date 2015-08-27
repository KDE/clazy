/*
   This file is part of the clang-lazy static checker.

  Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
  Author: Sérgio Martins <sergio.martins@kdab.com>

  Copyright (C) 2015 Sergio Martins <smartins@kde.org>

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

#include "duplicateexpensivestatement.h"
#include "Utils.h"
#include "checkmanager.h"

using namespace clang;
using namespace std;

static bool nameMatches(const std::string &qualifiedName)
{
    static vector<string> names = {"QHash::values", "QMap::values", "QSet::values", "QList::toVector", "QSet::toList", "QList::toSet", "QVector::toList"};
    return !qualifiedName.empty() && std::find(names.cbegin(), names.cend(), qualifiedName) != names.cend();
}

DuplicateExpensiveStatement::DuplicateExpensiveStatement(const std::string &name)
    : CheckBase(name)
{
}

void DuplicateExpensiveStatement::VisitDecl(Decl *decl)
{
   auto functionDecl = dyn_cast<FunctionDecl>(decl);
   if (functionDecl != nullptr) {
       m_currentFunctionDecl = functionDecl;
       inspectStatement(functionDecl->getBody());
   }
}

void DuplicateExpensiveStatement::inspectStatement(Stmt *stm)
{
    if (stm == nullptr)
        return;

    CXXMemberCallExpr *memberCallExpr = dyn_cast<CXXMemberCallExpr>(stm);
    if (memberCallExpr && memberCallExpr->getMethodDecl()) {
        CXXMethodDecl *methodDecl = dyn_cast<CXXMethodDecl>(memberCallExpr->getMethodDecl());
        if (methodDecl && methodDecl->getParent()) {
            std::string qualifiedName = methodDecl->getParent()->getNameAsString() + "::" + methodDecl->getNameAsString(); // memberExpr->getMemberDecl()->getQualifiedNameAsString() would return QMap<QString, QFoo>::values and we don't want the template arguments
            if (nameMatches(qualifiedName)) {
                ValueDecl *valueDecl = Utils::valueDeclForMemberCall(memberCallExpr);
                if (valueDecl) {
                    m_expensiveCounts[m_currentFunctionDecl][valueDecl]++;
                    if (m_expensiveCounts[m_currentFunctionDecl][valueDecl] > 1) {
                        emitWarning(memberCallExpr->getLocStart(), "Duplicate expensive statement");
                    }
                }
            }
        }
    }

    // recurse into the childs
    auto it = stm->child_begin();
    auto end = stm->child_end();
    for (; it != end; ++it) {
        inspectStatement(*it);
    }
}

// REGISTER_CHECK("duplicate-expensive-statement", DuplicateExpensiveStatement)
