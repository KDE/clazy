/*
    SPDX-FileCopyrightText: 2017 Sergio Martins <sergio.martins@kdab.com>
    SPDX-FileCopyrightText: 2024 Alexander Lohnau <alexander.lohnau@gmx.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "connect-3arg-lambda.h"
#include "ClazyContext.h"
#include "HierarchyUtils.h"
#include "QtUtils.h"

#include <clang/AST/Decl.h>
#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/Stmt.h>
#include <clang/Basic/LLVM.h>
#include <llvm/Support/Casting.h>

#include <vector>

class ClazyContext;

using namespace clang;

using uint = unsigned;

Connect3ArgLambda::Connect3ArgLambda(const std::string &name, ClazyContext *context)
    : CheckBase(name, context, Option_CanIgnoreIncludes)
{
}

void Connect3ArgLambda::VisitStmt(clang::Stmt *stmt)
{
    auto *callExpr = dyn_cast<CallExpr>(stmt);
    if (!callExpr) {
        return;
    }

    FunctionDecl *fdecl = callExpr->getDirectCallee();
    if (!fdecl) {
        return;
    }

    const uint numParams = fdecl->getNumParams();
    if (numParams != 2 && numParams != 3) {
        return;
    }

    std::string qualifiedName = fdecl->getQualifiedNameAsString();
    if (qualifiedName == "QTimer::singleShot") {
        processQTimer(fdecl, stmt);
        return;
    }

    if (qualifiedName == "QMenu::addAction") {
        processQMenu(fdecl, stmt);
        return;
    }
    if (qualifiedName == "QWidget::addAction") {
        processWidget(fdecl, stmt);
        return;
    }

    if (numParams != 3 || !clazy::isConnect(fdecl)) {
        return;
    }

    auto *arg3 = callExpr->getArg(2);
    auto *lambda = clang::dyn_cast_or_null<LambdaExpr>(arg3);
    if (!lambda) {
        lambda = clazy::getFirstChildOfType2<LambdaExpr>(arg3);
        if (!lambda) {
            return;
        }
    }

    DeclRefExpr *senderDeclRef = nullptr;
    MemberExpr *senderMemberExpr = nullptr;

    Stmt *s = callExpr->getArg(0);
    while (s) {
        if ((senderDeclRef = dyn_cast<DeclRefExpr>(s))) {
            break;
        }

        if ((senderMemberExpr = dyn_cast<MemberExpr>(s))) {
            break;
        }

        s = clazy::getFirstChild(s);
    }

    // The sender can be: this
    auto *senderThis = clazy::unpeal<CXXThisExpr>(callExpr->getArg(0), clazy::IgnoreImplicitCasts);

    // The variables used inside the lambda
    auto declrefs = clazy::getStatements<DeclRefExpr>(lambda->getBody());

    ValueDecl *senderDecl = senderDeclRef ? senderDeclRef->getDecl() : nullptr;

    // We'll only warn if the lambda is dereferencing another QObject (besides the sender)
    bool found = false;
    for (auto *declref : declrefs) {
        ValueDecl *decl = declref->getDecl();
        if (decl == senderDecl) {
            continue; // It's the sender, continue.
        }

        if (clazy::isQObject(decl->getType())) {
            found = true;
            break;
        }
    }

    if (!found) {
        auto thisexprs = clazy::getStatements<CXXThisExpr>(lambda->getBody());
        if (!thisexprs.empty() && !senderThis) {
            found = true;
        }
    }

    if (found) {
        emitWarning(stmt, "Pass a context object as 3rd connect parameter");
    }
}

void Connect3ArgLambda::processQTimer(FunctionDecl *func, Stmt *stmt)
{
    // Signatures to catch:
    // QTimer::singleShot(int msec, Functor functor)
    // QTimer::singleShot(int msec, Qt::TimerType timerType, Functor functor)

    const uint numParams = func->getNumParams();
    if (numParams == 2) {
        if (func->getParamDecl(0)->getNameAsString() == "interval" && func->getParamDecl(1)->getNameAsString() == "slot") {
            emitWarning(stmt, "Pass a context object as 2nd singleShot parameter");
        }
    } else if (numParams == 3) {
        if (func->getParamDecl(0)->getNameAsString() == "interval" && func->getParamDecl(1)->getNameAsString() == "timerType"
            && func->getParamDecl(2)->getNameAsString() == "slot") {
            emitWarning(stmt, "Pass a context object as 3rd singleShot parameter");
        }
    }
}

void Connect3ArgLambda::processQMenu(FunctionDecl *func, Stmt *stmt)
{
    // Signatures to catch:
    // QMenu::addAction(const QString &text, Func1 slot, const QKeySequence &shortcut = 0)
    const uint numParams = func->getNumParams();
    if (numParams == 3) {
        if (func->getParamDecl(0)->getNameAsString() == "text" && func->getParamDecl(1)->getNameAsString() == "slot"
            && func->getParamDecl(2)->getNameAsString() == "shortcut") {
            emitWarning(stmt, "Pass a context object as 2nd addAction parameter");
        }
    }
}

void Connect3ArgLambda::processWidget(clang::FunctionDecl *func, clang::Stmt *stmt)
{
    if (const uint numParams = func->getNumParams(); numParams > 1) {
        auto *possiblyFwdArgToConnect = func->getParamDecl(numParams - 2);
        auto *fwdArgToConnect = func->getParamDecl(numParams - 1);
        if (possiblyFwdArgToConnect->getNameAsString() != "args" && fwdArgToConnect->getNameAsString() == "args") {
            emitWarning(stmt, "Pass a context object as 2nd addAction parameter");
        }
    }
}
