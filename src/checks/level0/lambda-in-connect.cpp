/*
    SPDX-FileCopyrightText: 2016-2017 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "lambda-in-connect.h"
#include "ClazyContext.h"
#include "ContextUtils.h"
#include "HierarchyUtils.h"
#include "QtUtils.h"
#include "StringUtils.h"

#include <clang/AST/Decl.h>
#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/LambdaCapture.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/Type.h>
#include <clang/Basic/LLVM.h>
#include <clang/Basic/Lambda.h>
#include <llvm/ADT/iterator_range.h>
#include <llvm/Support/Casting.h>

using namespace clang;

LambdaInConnect::LambdaInConnect(const std::string &name, ClazyContext *context)
    : CheckBase(name, context, Option_CanIgnoreIncludes)
{
}

void LambdaInConnect::VisitStmt(clang::Stmt *stmt)
{
    auto *lambda = dyn_cast<LambdaExpr>(stmt);
    if (!lambda) {
        return;
    }

    auto captures = lambda->captures();
    if (captures.begin() == captures.end()) {
        return;
    }

    auto *callExpr = clazy::getFirstParentOfType<CallExpr>(m_context->parentMap, lambda);
    if (clazy::qualifiedMethodName(callExpr) != "QObject::connect") {
        return;
    }

    ValueDecl *senderDecl = clazy::signalSenderForConnect(callExpr);
    if (senderDecl) {
        const Type *t = senderDecl->getType().getTypePtrOrNull();
        if (t && !t->isPointerType()) {
            return;
        }
    }

    ValueDecl *receiverDecl = clazy::signalReceiverForConnect(callExpr);
    if (receiverDecl) {
        const Type *t = receiverDecl->getType().getTypePtrOrNull();
        if (t && !t->isPointerType()) {
            return;
        }
    }

    for (auto capture : captures) {
        if (capture.getCaptureKind() == clang::LCK_ByRef) {
            auto *declForCapture = capture.getCapturedVar();
            if (declForCapture && declForCapture != receiverDecl && clazy::isValueDeclInFunctionContext(declForCapture)) {
                emitWarning(capture.getLocation(), "captured local variable by reference might go out of scope before lambda is called");
            }
        }
    }
}
