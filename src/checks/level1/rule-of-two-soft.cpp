/*
    SPDX-FileCopyrightText: 2015 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "rule-of-two-soft.h"

#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/Stmt.h>
#include <clang/Basic/LLVM.h>
#include <llvm/Support/Casting.h>

using namespace clang;

void RuleOfTwoSoft::VisitStmt(Stmt *s)
{
    if (auto *op = dyn_cast<CXXOperatorCallExpr>(s)) {
        FunctionDecl *func = op->getDirectCallee();
        auto *method = func ? dyn_cast<CXXMethodDecl>(func) : nullptr;
        if (method && method->getParent() && method->isCopyAssignmentOperator()) {
            CXXRecordDecl *record = method->getParent();
            const bool hasCopyCtor = record->hasNonTrivialCopyConstructor();
            const bool hasCopyAssignOp = record->hasNonTrivialCopyAssignment() || method->isExplicitlyDefaulted();

            if (hasCopyCtor && !hasCopyAssignOp && !isBlacklisted(record)) {
                std::string msg = "Using assign operator but class " + record->getQualifiedNameAsString() + " has copy-ctor but no assign operator";
                emitWarning(s->getBeginLoc(), msg);
            }
        }
    } else if (auto *ctorExpr = dyn_cast<CXXConstructExpr>(s)) {
        CXXConstructorDecl *ctorDecl = ctorExpr->getConstructor();
        CXXRecordDecl *record = ctorDecl->getParent();
        if (ctorDecl->isCopyConstructor() && record) {
            const bool hasCopyCtor = record->hasNonTrivialCopyConstructor() || ctorDecl->isExplicitlyDefaulted();
            const bool hasCopyAssignOp = record->hasNonTrivialCopyAssignment();
            if (!hasCopyCtor && hasCopyAssignOp && !isBlacklisted(record)) {
                std::string msg =
                    "Using copy-ctor but class " + record->getQualifiedNameAsString() + " has a trivial copy-ctor but non trivial assign operator";
                emitWarning(s->getBeginLoc(), msg);
            }
        }
    }
}
