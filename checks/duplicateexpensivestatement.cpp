/**********************************************************************
**  Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
**  Author: Sérgio Martins <sergio.martins@kdab.com>
**
** This file may be distributed and/or modified under the terms of the
** GNU Lesser General Public License version 2.1 and version 3 as published by the
** Free Software Foundation and appearing in the file LICENSE.LGPL.txt included.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**********************************************************************/

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
