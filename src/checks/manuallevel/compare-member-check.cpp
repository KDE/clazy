/*
    This file is part of the clazy static checker.

    Copyright (C) 2023 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
    Author: Shivam Kunwar <shivam.kunwar@kdab.com>

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

#include "compare-member-check.h"
#include <clang/AST/AST.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/ParentMap.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/ASTMatchers/ASTMatchersInternal.h>
#include <iostream>

using namespace clang::ast_matchers;
class ClazyContext;

using namespace clang;

void checkBinaryOperator(const BinaryOperator *binOp, const CXXRecordDecl *record, std::vector<const FieldDecl *> &classFields)
{
    if (const BinaryOperator *leftBO = llvm::dyn_cast<BinaryOperator>(binOp->getLHS())) {
        checkBinaryOperator(leftBO, record, classFields);
    }

    if (const BinaryOperator *rightBO = llvm::dyn_cast<BinaryOperator>(binOp->getRHS())) {
        checkBinaryOperator(rightBO, record, classFields);
    }

    const MemberExpr *lhsMemberExpr = nullptr;
    if (const ImplicitCastExpr *implicitCast = dyn_cast<ImplicitCastExpr>(binOp->getLHS())) {
        lhsMemberExpr = dyn_cast<MemberExpr>(implicitCast->getSubExpr());
    } else if (const MemberExpr *temp = dyn_cast<MemberExpr>(binOp->getLHS())) {
        lhsMemberExpr = temp;
    }

    if (lhsMemberExpr) {
        FieldDecl *fieldDecl = dyn_cast<FieldDecl>(lhsMemberExpr->getMemberDecl());
        if (fieldDecl && fieldDecl->getParent() == record) {
            auto it = std::find(classFields.begin(), classFields.end(), fieldDecl);

            if (it != classFields.end()) {
                classFields.erase(it);
            }
        }
    }
}

class Caller : public ClazyAstMatcherCallback
{
public:
    Caller(CheckBase *check)
        : ClazyAstMatcherCallback(check)
    {
    }
    virtual void run(const MatchFinder::MatchResult &result)
    {
        if (const CXXOperatorCallExpr *callExpr = result.Nodes.getNodeAs<CXXOperatorCallExpr>("callExpr")) {
            const clang::Decl *calleeDecl = callExpr->getCalleeDecl();
            if (!calleeDecl)
                return;

            const auto *methodDecl = dyn_cast<CXXMethodDecl>(calleeDecl);
            if (!methodDecl)
                return;

            // Get the class declaration
            const CXXRecordDecl *classDecl = methodDecl->getParent();

            std::vector<const FieldDecl *> classFields;
            for (const auto *field : classDecl->fields()) {
                classFields.push_back(field);
            }
            for (const auto *stmt : methodDecl->getBody()->children()) {
                const ReturnStmt *returnStmt = dyn_cast<ReturnStmt>(stmt);
                if (!returnStmt)
                    return;
                const Expr *returnExpr = returnStmt->getRetValue();
                if (const clang::ExprWithCleanups *exprWithCleanups = dyn_cast<ExprWithCleanups>(returnExpr)) {
                    if (auto opCallExpr = llvm::dyn_cast<clang::CXXOperatorCallExpr>(exprWithCleanups->getSubExpr())) {
                        const clang::Expr *lhs = opCallExpr->getArg(0);
                        if (auto *materializeExpr = llvm::dyn_cast<MaterializeTemporaryExpr>(lhs)) {
                            if (auto *implicitCastExpr = llvm::dyn_cast<ImplicitCastExpr>(materializeExpr->getSubExpr())) {
                                if (auto *lhsCall = dyn_cast<CallExpr>(implicitCastExpr->getSubExpr())) {
                                    const auto &lhsArgs = lhsCall->arguments();

                                    size_t numLhsArgs = std::distance(lhsArgs.begin(), lhsArgs.end());
                                    if (classFields.size() != numLhsArgs) {
                                        m_check->emitWarning(callExpr->getExprLoc(), "Comparison operator does not use all member variables");
                                    }
                                }
                            }
                        }
                    }
                }

                if (const BinaryOperator *binOp = dyn_cast<BinaryOperator>(returnExpr)) {
                    checkBinaryOperator(binOp, classDecl, classFields);
                    if (!classFields.empty()) {
                        m_check->emitWarning(callExpr->getExprLoc(), "Comparison operator does not use all member variables");
                    }
                }
            }
        }
    }
};

CompareMemberCheck::CompareMemberCheck(const std::string &name, ClazyContext *context)
    : CheckBase(name, context, Option_CanIgnoreIncludes)
    , m_astMatcherCallBack(std::make_unique<Caller>(this))
{
}

CompareMemberCheck::~CompareMemberCheck() = default;

void CompareMemberCheck::VisitStmt(Stmt *stmt)
{
    auto call = dyn_cast<CXXOperatorCallExpr>(stmt);
    if (!call || call->getNumArgs() != 1)
        return;
}

void CompareMemberCheck::registerASTMatchers(MatchFinder &finder)
{
    finder.addMatcher(cxxOperatorCallExpr().bind("callExpr"), m_astMatcherCallBack.get());
}