/*
    SPDX-FileCopyrightText: 2015 Klarälvdalens Datakonsult AB a KDAB Group company info@kdab.com
    SPDX-FileContributor: Sérgio Martins <sergio.martins@kdab.com>

    SPDX-FileCopyrightText: 2015 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "LoopUtils.h"
#include "StringUtils.h"

#include <clang/AST/Decl.h>
#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/ParentMap.h>
#include <clang/AST/StmtCXX.h>
#include <clang/Basic/LLVM.h>
#include <clang/Basic/SourceLocation.h>
#include <llvm/ADT/StringRef.h>

namespace clang
{
class CXXConstructorDecl;
} // namespace clang

using namespace clang;

Stmt *clazy::bodyFromLoop(Stmt *loop)
{
    if (!loop) {
        return nullptr;
    }

    if (auto *forstm = dyn_cast<ForStmt>(loop)) {
        return forstm->getBody();
    }

    if (auto *rangeLoop = dyn_cast<CXXForRangeStmt>(loop)) {
        return rangeLoop->getBody();
    }

    if (auto *whilestm = dyn_cast<WhileStmt>(loop)) {
        return whilestm->getBody();
    }

    if (auto *dostm = dyn_cast<DoStmt>(loop)) {
        return dostm->getBody();
    }

    return nullptr;
}

bool clazy::loopCanBeInterrupted(clang::Stmt *stmt, const clang::SourceManager &sm, clang::SourceLocation onlyBeforeThisLoc)
{
    if (!stmt) {
        return false;
    }

    if (isa<ReturnStmt>(stmt) || isa<BreakStmt>(stmt) || isa<ContinueStmt>(stmt)) {
        if (onlyBeforeThisLoc.isValid()) {
            FullSourceLoc sourceLoc(stmt->getBeginLoc(), sm);
            FullSourceLoc otherSourceLoc(onlyBeforeThisLoc, sm);
            if (sourceLoc.isBeforeInTranslationUnitThan(otherSourceLoc)) {
                return true;
            }
        } else {
            return true;
        }
    }

    return clazy::any_of(stmt->children(), [&sm, onlyBeforeThisLoc](Stmt *s) {
        return clazy::loopCanBeInterrupted(s, sm, onlyBeforeThisLoc);
    });
}

clang::Expr *clazy::containerExprForLoop(Stmt *loop, const std::string &qtNamespace)
{
    if (!loop) {
        return nullptr;
    }

    if (auto *rangeLoop = dyn_cast<CXXForRangeStmt>(loop)) {
        return rangeLoop->getRangeInit();
    }

    // foreach with C++14
    if (auto *constructExpr = dyn_cast<CXXConstructExpr>(loop)) {
        if (constructExpr->getNumArgs() < 1) {
            return nullptr;
        }

        const CXXConstructorDecl *constructorDecl = constructExpr->getConstructor();
        if (!constructorDecl || constructorDecl->getQualifiedNameAsString() != clazy::qtNamespaced("QForeachContainer", qtNamespace)) {
            return nullptr;
        }

        return constructExpr;
    }

    // foreach with C++17
    if (auto *bindTempExpr = dyn_cast<CXXBindTemporaryExpr>(loop)) {
        auto *callExpr = dyn_cast<CallExpr>(bindTempExpr->getSubExpr());
        if (!callExpr)
            return nullptr;
        FunctionDecl *func = callExpr->getDirectCallee();
        if (!func || func->getQualifiedNameAsString() != clazy::qtNamespaced("QtPrivate::qMakeForeachContainer", qtNamespace)) {
            return nullptr;
        }
        if (callExpr->getNumArgs() < 1) {
            return nullptr;
        }
        return callExpr->getArg(0);
    }

    return nullptr;
}

VarDecl *clazy::containerDeclForLoop(clang::Stmt *loop, const std::string &qtNamespace)
{
    Expr *expr = containerExprForLoop(loop, qtNamespace);
    if (!expr) {
        return nullptr;
    }

    auto *declRef = dyn_cast<DeclRefExpr>(expr);
    if (!declRef) {
        return nullptr;
    }

    ValueDecl *valueDecl = declRef->getDecl();
    return valueDecl ? dyn_cast<VarDecl>(valueDecl) : nullptr;
}

Stmt *clazy::isInLoop(clang::ParentMap *pmap, clang::Stmt *stmt)
{
    if (!stmt) {
        return nullptr;
    }

    Stmt *p = pmap->getParent(stmt);
    while (p) {
        if (clazy::isLoop(p)) {
            return p;
        }
        p = pmap->getParent(p);
    }

    return nullptr;
}
