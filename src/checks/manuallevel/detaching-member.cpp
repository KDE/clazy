/*
    SPDX-FileCopyrightText: 2015 Klarälvdalens Datakonsult AB a KDAB Group company info@kdab.com
    SPDX-FileContributor: Sérgio Martins <sergio.martins@kdab.com>

    SPDX-FileCopyrightText: 2015 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "detaching-member.h"
#include "ClazyContext.h"
#include "HierarchyUtils.h"
#include "StringUtils.h"
#include "Utils.h"
#include "checkbase.h"
#include "clazy_stl.h"

#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/Type.h>
#include <clang/Basic/LLVM.h>
#include <clang/Basic/OperatorKinds.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Casting.h>

#include <vector>

using namespace clang;

DetachingMember::DetachingMember(const std::string &name, ClazyContext *context)
    : DetachingBase(name, context, Option_CanIgnoreIncludes)
{
    m_filesToIgnore = {"qstring.h"};
}

void DetachingMember::VisitStmt(clang::Stmt *stm)
{
    auto *callExpr = dyn_cast<CallExpr>(stm);
    if (!callExpr) {
        return;
    }

    auto *memberCall = dyn_cast<CXXMemberCallExpr>(callExpr);
    auto *operatorExpr = dyn_cast<CXXOperatorCallExpr>(callExpr);
    if (!memberCall && !operatorExpr) {
        return;
    }

    if (shouldIgnoreFile(stm->getBeginLoc())) {
        return;
    }

    CXXMethodDecl *method = nullptr;
    ValueDecl *memberDecl = nullptr;
    if (operatorExpr) {
        FunctionDecl *func = operatorExpr->getDirectCallee();
        method = func ? dyn_cast<CXXMethodDecl>(func) : nullptr;
        if (!method || method->getOverloadedOperator() != clang::OO_Subscript) {
            return;
        }

        auto *memberExpr = clazy::getFirstParentOfType<CXXMemberCallExpr>(m_context->parentMap, operatorExpr);
        CXXMethodDecl *parentMemberDecl = memberExpr ? memberExpr->getMethodDecl() : nullptr;
        if (parentMemberDecl && !parentMemberDecl->isConst()) {
            // Don't warn for s.m_listOfValues[0].nonConstMethod();
            // However do warn for: s.m_listOfPointers[0]->nonConstMethod(); because it compiles with .at()
            QualType qt = operatorExpr->getType();
            const Type *t = qt.getTypePtrOrNull();
            if (t && !t->isPointerType()) {
                return;
            }
        }

        memberDecl = Utils::valueDeclForOperatorCall(operatorExpr);
    } else {
        method = memberCall->getMethodDecl();
        memberDecl = Utils::valueDeclForMemberCall(memberCall);
    }

    if (!method || !memberDecl || !Utils::isMemberVariable(memberDecl) || !isDetachingMethod(method, DetachingMethodWithConstCounterPart)
        || method->isConst()) {
        return;
    }

    // Catch cases like m_foo[0] = .. , which is fine

    auto *parentUnaryOp = clazy::getFirstParentOfType<UnaryOperator>(m_context->parentMap, callExpr);
    if (parentUnaryOp) {
        // m_foo[0]++ is OK
        return;
    }

    auto *parentOp = clazy::getFirstParentOfType<CXXOperatorCallExpr>(m_context->parentMap, clazy::parent(m_context->parentMap, callExpr));
    if (parentOp) {
        FunctionDecl *parentFunc = parentOp->getDirectCallee();
        const std::string parentFuncName = parentFunc ? parentFunc->getNameAsString() : "";
        if (clazy::startsWith(parentFuncName, "operator")) {
            // m_foo[0] = ... is OK
            return;
        }
    }

    auto *parentBinaryOp = clazy::getFirstParentOfType<BinaryOperator>(m_context->parentMap, callExpr);
    if (parentBinaryOp && parentBinaryOp->isAssignmentOp()) {
        // m_foo[0] += .. is OK
        Expr *lhs = parentBinaryOp->getLHS();
        if (callExpr == lhs || clazy::isChildOf(callExpr, lhs)) {
            return;
        }
    }

    const bool returnsNonConstIterator = memberCall && clazy::endsWith(memberCall->getType().getAsString(lo()), "iterator");
    if (returnsNonConstIterator) {
        // If we're calling begin()/end() as arguments to a function taking non-const iterators it's fine
        // Such as qSort(list.begin(), list.end());
        auto *parentCall = clazy::getFirstParentOfType<CallExpr>(m_context->parentMap, clazy::parent(m_context->parentMap, memberCall));
        FunctionDecl *parentFunc = parentCall ? parentCall->getDirectCallee() : nullptr;
        if (parentFunc && parentFunc->getNumParams() == parentCall->getNumArgs()) {
            int i = 0;
            for (auto *argExpr : parentCall->arguments()) {
                auto expr2 = dyn_cast<CXXMemberCallExpr>(argExpr); // C++17
                if (!expr2)
                    expr2 = clazy::getFirstChildOfType<CXXMemberCallExpr>(argExpr); // C++14
                if (expr2) {
                    if (expr2 == memberCall) {
                        // Success, we found which arg
                        ParmVarDecl *param = parentFunc->getParamDecl(i);

                        // Check that the record declarations have the same type
                        if (param->getType().getTypePtr()->getAsRecordDecl()->getNameAsString()
                            == memberCall->getType().getTypePtr()->getAsRecordDecl()->getNameAsString()) {
                            return;
                        }
                        break;
                    }
                }
                ++i;
            }
        }
    }

    emitWarning(stm->getBeginLoc(), "Potential detachment due to calling " + method->getQualifiedNameAsString() + "()");
}
