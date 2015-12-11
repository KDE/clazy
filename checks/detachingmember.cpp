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

#include "checkmanager.h"
#include "detachingmember.h"
#include "Utils.h"
#include "HierarchyUtils.h"
#include "StringUtils.h"

#include <clang/AST/DeclCXX.h>
#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>

using namespace clang;
using namespace std;

DetachingMember::DetachingMember(const std::string &name)
    : DetachingBase(name)
{
}

void DetachingMember::VisitStmt(clang::Stmt *stm)
{
    CallExpr *callExpr = dyn_cast<CallExpr>(stm);
    if (!callExpr)
        return;

    CXXMemberCallExpr *memberCall = dyn_cast<CXXMemberCallExpr>(callExpr);
    CXXOperatorCallExpr *operatorExpr = dyn_cast<CXXOperatorCallExpr>(callExpr);
    if (!memberCall && !operatorExpr)
        return;

    CXXMethodDecl *method = nullptr;
    ValueDecl *memberDecl = nullptr;
    if (operatorExpr) {
        FunctionDecl *func = operatorExpr->getDirectCallee();
        method = func ? dyn_cast<CXXMethodDecl>(func) : nullptr;
        if (!method || method->getNameAsString() != "operator[]")
            return;

        memberDecl = Utils::valueDeclForOperatorCall(operatorExpr);

    } else {
        method = memberCall->getMethodDecl();
        memberDecl = Utils::valueDeclForMemberCall(memberCall);
    }

    if (!method || !memberDecl || !Utils::isMemberVariable(memberDecl) || !isDetachingMethod(method) || method->isConst())
        return;

    // Catch cases like m_foo[0] = .. , which is fine

    auto parentUnaryOp = HierarchyUtils::getFirstParentOfType<UnaryOperator>(m_parentMap, callExpr);
    if (parentUnaryOp) {
        // m_foo[0]++ is OK
        return;
    }

    auto parentOp = HierarchyUtils::getFirstParentOfType<CXXOperatorCallExpr>(m_parentMap, HierarchyUtils::parent(m_parentMap, callExpr));
    if (parentOp) {
        FunctionDecl *parentFunc = parentOp->getDirectCallee();
        if (parentFunc && parentFunc->getNameAsString() == "operator=") {
            // m_foo[0] = ... is OK
            return;
        }
    }

    auto parentBinaryOp = HierarchyUtils::getFirstParentOfType<BinaryOperator>(m_parentMap, callExpr);
    if (parentBinaryOp && parentBinaryOp->isAssignmentOp()) {
        // m_foo[0] += .. is OK
        Expr *lhs = parentBinaryOp->getLHS();
        if (callExpr == lhs || Utils::isChildOf(callExpr, lhs))
            return;
    }

    emitWarning(stm->getLocStart(), "Potential detachment due to calling " + method->getQualifiedNameAsString() + "()");
}

std::vector<std::string> DetachingMember::filesToIgnore() const
{
    static const vector<string> files = {"qstring.h"};
    return files;
}

REGISTER_CHECK_WITH_FLAGS("detaching-member", DetachingMember, CheckLevel3)
