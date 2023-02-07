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

#include "unused-result-check.h"

#include <clang/AST/AST.h>
#include <clang/AST/ExprCXX.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/ASTMatchers/ASTMatchersInternal.h>

using namespace clang::ast_matchers;
class ClazyContext;

using namespace clang;

class Caller : public ClazyAstMatcherCallback
{
public:
    Caller(CheckBase *check)
        : ClazyAstMatcherCallback(check)
    {
    }
    void run(const MatchFinder::MatchResult &result) override
    {
        if (const auto *callExpr = result.Nodes.getNodeAs<CXXMemberCallExpr>("callExpr")) {
            if (callExpr->getMethodDecl()->isConst() && !callExpr->getMethodDecl()->getReturnType()->isVoidType()) {
                const auto &parents = result.Context->getParents(*callExpr);

                if (parents[0].get<Stmt>() != nullptr && parents[0].get<Decl>() == nullptr) {
                    if (!llvm::dyn_cast<Expr>(parents[0].get<Stmt>()) && !llvm::dyn_cast<ReturnStmt>(parents[0].get<Stmt>())
                        && !llvm::dyn_cast<IfStmt>(parents[0].get<Stmt>()) && !llvm::dyn_cast<WhileStmt>(parents[0].get<Stmt>())
                        && !llvm::dyn_cast<DoStmt>(parents[0].get<Stmt>()) && !llvm::dyn_cast<SwitchStmt>(parents[0].get<Stmt>())
                        && !llvm::dyn_cast<ForStmt>(parents[0].get<Stmt>()) && !llvm::dyn_cast<CXXThisExpr>(parents[0].get<Stmt>())) {
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

UnusedResultCheck::UnusedResultCheck(const std::string &name, ClazyContext *context)
    : CheckBase(name, context, Option_CanIgnoreIncludes)
    , m_astMatcherCallBack(std::make_unique<Caller>(this))
{
}

UnusedResultCheck::~UnusedResultCheck() = default;

void UnusedResultCheck::VisitStmt(Stmt *stmt)
{
    auto *call = dyn_cast<CXXMemberCallExpr>(stmt);
    if (!call || call->getNumArgs() != 1) {
        return;
    }
}

void UnusedResultCheck::registerASTMatchers(MatchFinder &finder)
{
    finder.addMatcher(cxxMemberCallExpr().bind("callExpr"), m_astMatcherCallBack.get());
}
