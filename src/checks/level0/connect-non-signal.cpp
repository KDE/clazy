/*
    SPDX-FileCopyrightText: 2016 Sergio Martins <smartins@kde.org>
    SPDX-FileCopyrightText: 2016 Klarälvdalens Datakonsult AB a KDAB Group company info@kdab.coms

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "connect-non-signal.h"
#include "AccessSpecifierManager.h"
#include "ClazyContext.h"
#include "QtUtils.h"

#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/Expr.h>
#include <clang/AST/Stmt.h>
#include <clang/Basic/LLVM.h>
#include <llvm/Support/Casting.h>

using namespace clang;

ConnectNonSignal::ConnectNonSignal(const std::string &name, ClazyContext *context)
    : CheckBase(name, context, Option_CanIgnoreIncludes)
{
    context->enableAccessSpecifierManager();
}

void ConnectNonSignal::VisitStmt(clang::Stmt *stmt)
{
    auto *call = dyn_cast<CallExpr>(stmt);
    if (!call) {
        return;
    }

    FunctionDecl *func = call->getDirectCallee();
    if (!clazy::isConnect(func) || !clazy::connectHasPMFStyle(func)) {
        return;
    }

    CXXMethodDecl *method = clazy::pmfFromConnect(call, /*argIndex=*/1);
    if (!method) {
        if (clazy::isQMetaMethod(call, 1)) {
            return;
        }
        emitWarning(call->getBeginLoc(), "couldn't find method from pmf connect, please report a bug");
        return;
    }

    const AccessSpecifierManager *accessSpecifierManager = m_context->accessSpecifierManager;
    if (!accessSpecifierManager) {
        return;
    }

    QtAccessSpecifierType qst = accessSpecifierManager->qtAccessSpecifierType(method);
    if (qst != QtAccessSpecifier_Unknown && qst != QtAccessSpecifier_Signal) {
        emitWarning(call, method->getQualifiedNameAsString() + std::string(" is not a signal"));
    }
}
