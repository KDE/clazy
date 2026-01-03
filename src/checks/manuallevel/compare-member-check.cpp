/*
    SPDX-FileCopyrightText: 2023 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.com>
    SPDX-FileContributor: Shivam Kunwar <shivam.kunwar@kdab.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "compare-member-check.h"
#include <clang/AST/AST.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/ParentMap.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/ASTMatchers/ASTMatchersInternal.h>

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

CompareMemberCheck::~CompareMemberCheck() = default;

void CompareMemberCheck::registerASTMatchers(MatchFinder &finder)
{
    m_astMatcherCallBack = std::make_unique<Caller>(this);
    finder.addMatcher(cxxOperatorCallExpr().bind("callExpr"), m_astMatcherCallBack.get());
}
