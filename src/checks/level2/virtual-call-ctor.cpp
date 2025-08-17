/*
    SPDX-FileCopyrightText: 2015 Klarälvdalens Datakonsult AB a KDAB Group company info@kdab.com
    SPDX-FileContributor: Sérgio Martins <sergio.martins@kdab.com>

    SPDX-FileCopyrightText: 2015 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "virtual-call-ctor.h"
#include "ClazyContext.h"
#include "clazy_stl.h"

#include <clang/AST/DeclBase.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/Stmt.h>
#include <clang/Basic/LLVM.h>
#include <clang/Basic/SourceLocation.h>
#include <llvm/Support/Casting.h>

using namespace clang;

namespace clazy
{
void getChildsIgnoreLambda(clang::Stmt *stmt, std::vector<CXXMemberCallExpr *> &result_list, int depth = -1)
{
    if (!stmt || dyn_cast<LambdaExpr>(stmt)) {
        return;
    }

    auto *cexpr = llvm::dyn_cast<CXXMemberCallExpr>(stmt);
    if (cexpr) {
        result_list.push_back(cexpr);
    }

    if (depth > 0 || depth == -1) {
        if (depth > 0) {
            --depth;
        }
        for (auto *child : stmt->children()) {
            getChildsIgnoreLambda(child, result_list, depth);
        }
    }
}
}

void VirtualCallCtor::VisitDecl(Decl *decl)
{
    auto *ctorDecl = dyn_cast<CXXConstructorDecl>(decl);
    auto *dtorDecl = dyn_cast<CXXDestructorDecl>(decl);
    if (!ctorDecl && !dtorDecl) {
        return;
    }

    Stmt *ctorOrDtorBody = ctorDecl ? ctorDecl->getBody() : dtorDecl->getBody();
    if (!ctorOrDtorBody) {
        return;
    }

    CXXRecordDecl *classDecl = ctorDecl ? ctorDecl->getParent() : dtorDecl->getParent();

    std::vector<Stmt *> processedStmts;
    SourceLocation loc = containsVirtualCall(classDecl, ctorOrDtorBody, processedStmts);
    if (loc.isValid()) {
        if (ctorDecl) {
            emitWarning(decl->getBeginLoc(), "Calling pure virtual function in CTOR");
        } else {
            emitWarning(decl->getBeginLoc(), "Calling pure virtual function in DTOR");
        }
        emitWarning(loc, "Called here");
    }
}

SourceLocation VirtualCallCtor::containsVirtualCall(clang::CXXRecordDecl *classDecl, clang::Stmt *stmt, std::vector<Stmt *> &processedStmts)
{
    if (!stmt) {
        return {};
    }

    // already processed ? we don't want recurring calls
    if (clazy::contains(processedStmts, stmt)) {
        return {};
    }

    processedStmts.push_back(stmt);

    std::vector<CXXMemberCallExpr *> memberCalls;

    // Ignore lambdas, as they are usually used in connects. Might introduce true-negatives. Possible solution would be to check if the lambda is in a connect,
    // timer, invokeMethod, etc.
    clazy::getChildsIgnoreLambda(stmt, memberCalls);

    for (CXXMemberCallExpr *callExpr : memberCalls) {
        CXXMethodDecl *memberDecl = callExpr->getMethodDecl();
        if (!memberDecl || !isa<CXXThisExpr>(callExpr->getImplicitObjectArgument())) {
            continue;
        }

        if (memberDecl->getParent() == classDecl) {
            if (memberDecl->isPureVirtual()) {
                return callExpr->getBeginLoc();
            }
            if (containsVirtualCall(classDecl, memberDecl->getBody(), processedStmts).isValid()) {
                return callExpr->getBeginLoc();
            }
        }
    }

    return {};
}
