/*
    SPDX-FileCopyrightText: 2023 Klarälvdalens Datakonsult AB a KDAB Group company info@kdab.com
    SPDX-FileContributor: Shivam Kunwar <shivam.kunwar@kdab.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "unused-result-check.h"
#include "StringUtils.h"

#include <clang/AST/AST.h>
#include <clang/AST/ExprCXX.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/ASTMatchers/ASTMatchersInternal.h>

using namespace clang::ast_matchers;

using namespace clang;

class Caller : public ClazyAstMatcherCallback
{
public:
    explicit Caller(CheckBase *check)
        : ClazyAstMatcherCallback(check)
    {
    }
    void run(const MatchFinder::MatchResult &result) override
    {
        if (const auto *callExpr = result.Nodes.getNodeAs<CXXMemberCallExpr>("callExpr")) {
            if (const auto *methodDecl = callExpr->getMethodDecl(); methodDecl && methodDecl->isConst() && !methodDecl->getReturnType()->isVoidType()) {
                const auto &parents = result.Context->getParents(*callExpr);

                if (parents[0].get<Stmt>() != nullptr && parents[0].get<Decl>() == nullptr) {
                    if (!llvm::dyn_cast<Expr>(parents[0].get<Stmt>()) && !llvm::dyn_cast<ReturnStmt>(parents[0].get<Stmt>())
                        && !llvm::dyn_cast<IfStmt>(parents[0].get<Stmt>()) && !llvm::dyn_cast<WhileStmt>(parents[0].get<Stmt>())
                        && !llvm::dyn_cast<DoStmt>(parents[0].get<Stmt>()) && !llvm::dyn_cast<SwitchStmt>(parents[0].get<Stmt>())
                        && !llvm::dyn_cast<ForStmt>(parents[0].get<Stmt>())
                        && !llvm::dyn_cast<CXXThisExpr>(parents[0].get<Stmt>())
                        // Ignore this false positive from a Qt header for now, we pass a mutable pointer to a static function
                        && clazy::qualifiedMethodName(callExpr->getMethodDecl()) != "QMetaType::registerHelper") {
                        m_check->emitWarning(callExpr->getExprLoc(), "Result of const member function is not used.");
                    }
                }

                else if (parents[0].get<Decl>() != nullptr && parents[0].get<Stmt>() == nullptr) {
                    if (!llvm::dyn_cast<VarDecl>(parents[0].get<Decl>()) && !llvm::dyn_cast<CXXConstructorDecl>(parents[0].get<Decl>())) {
                        m_check->emitWarning(callExpr->getExprLoc(), "Result of const member function is not used.");
                    }
                }
            }
        }
    }
};

UnusedResultCheck::UnusedResultCheck(const std::string &name)
    : CheckBase(name, Option_CanIgnoreIncludes)
{
}

UnusedResultCheck::~UnusedResultCheck() = default;

void UnusedResultCheck::registerASTMatchers(MatchFinder &finder)
{
    m_astMatcherCallBack = std::make_unique<Caller>(this);
    finder.addMatcher(cxxMemberCallExpr().bind("callExpr"), m_astMatcherCallBack.get());
}
