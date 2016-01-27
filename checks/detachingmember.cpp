/*
   This file is part of the clazy static checker.

  Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
  Author: Sérgio Martins <sergio.martins@kdab.com>

  Copyright (C) 2015 Sergio Martins <smartins@kde.org>

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

DetachingMember::DetachingMember(const std::string &name, const clang::CompilerInstance &ci)
    : DetachingBase(name, ci)
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

        auto memberExpr = HierarchyUtils::getFirstParentOfType<CXXMemberCallExpr>(m_parentMap, operatorExpr);
        CXXMethodDecl *parentMemberDecl = memberExpr ? memberExpr->getMethodDecl() : nullptr;
        if (parentMemberDecl && !parentMemberDecl->isConst()) {
            // Don't warn for s.m_listOfValues[0].nonConstMethod();
            // However do warn for: s.m_listOfPointers[0]->nonConstMethod(); because it compiles with .at()
            QualType qt = operatorExpr->getType();
            const Type *t = qt.getTypePtrOrNull();
            if (t && !t->isPointerType())
                return;
        }

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
        const string parentFuncName = parentFunc ? parentFunc->getNameAsString() : "";
        if (stringStartsWith(parentFuncName, "operator")) {
            // m_foo[0] = ... is OK
            return;
        }
    }

    auto parentBinaryOp = HierarchyUtils::getFirstParentOfType<BinaryOperator>(m_parentMap, callExpr);
    if (parentBinaryOp && parentBinaryOp->isAssignmentOp()) {
        // m_foo[0] += .. is OK
        Expr *lhs = parentBinaryOp->getLHS();
        if (callExpr == lhs || HierarchyUtils::isChildOf(callExpr, lhs))
            return;
    }

    const bool returnsNonConstIterator = stringEndsWith(memberCall ? memberCall->getType().getAsString() : "", "::iterator");
    if (returnsNonConstIterator) {
        // If we're calling begin()/end() as arguments to a function taking non-const iterators it's fine
        // Such as qSort(list.begin(), list.end());
        auto parentCall = HierarchyUtils::getFirstParentOfType<CallExpr>(m_parentMap, HierarchyUtils::parent(m_parentMap, memberCall));
        FunctionDecl *parentFunc = parentCall ? parentCall->getDirectCallee() : nullptr;
        if (parentFunc && parentFunc->getNumParams() == parentCall->getNumArgs()) {
            int i = 0;
            for (auto argExpr : parentCall->arguments()) {
                if (CXXMemberCallExpr *expr2 = HierarchyUtils::getFirstChildOfType<CXXMemberCallExpr>(argExpr)) {
                    if (expr2 == memberCall) {
                        // Success, we found which arg
                        ParmVarDecl *parm = parentFunc->getParamDecl(i);
                        if (parm->getType().getAsString() == memberCall->getType().getAsString()) {
                            return;
                        } else {
                            break;
                        }
                    }
                }
                ++i;
            }
        }
    }

    emitWarning(stm->getLocStart(), "Potential detachment due to calling " + method->getQualifiedNameAsString() + "()");
}

std::vector<std::string> DetachingMember::filesToIgnore() const
{
    static const vector<string> files = {"qstring.h"};
    return files;
}

REGISTER_CHECK_WITH_FLAGS("detaching-member", DetachingMember, CheckLevel3)
