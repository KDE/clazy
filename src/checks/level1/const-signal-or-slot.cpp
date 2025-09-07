/*
    SPDX-FileCopyrightText: 2017 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "const-signal-or-slot.h"
#include "AccessSpecifierManager.h"
#include "ClazyContext.h"
#include "QtUtils.h"
#include "TypeUtils.h"

#include <clang/AST/DeclCXX.h>
#include <clang/AST/Expr.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/Type.h>
#include <clang/Basic/LLVM.h>
#include <llvm/Support/Casting.h>

using namespace clang;

void ConstSignalOrSlot::VisitStmt(clang::Stmt *stmt)
{
    auto *call = dyn_cast<CallExpr>(stmt);
    const AccessSpecifierManager *accessSpecifierManager = m_context->accessSpecifierManager;
    if (!call || !accessSpecifierManager) {
        return;
    }

    FunctionDecl *func = call->getDirectCallee();
    if (!clazy::isConnect(func, m_context->qtNamespace()) || !clazy::connectHasPMFStyle(func)) {
        return;
    }

    CXXMethodDecl *slot = clazy::receiverMethodForConnect(call);
    if (!slot || !slot->isConst() || slot->getReturnType()->isVoidType()) { // const and returning void must do something, so not a getter
        return;
    }

    QtAccessSpecifierType specifierType = accessSpecifierManager->qtAccessSpecifierType(slot);
    if (specifierType == QtAccessSpecifier_Slot || specifierType == QtAccessSpecifier_Signal) {
        return; // For stuff explicitly marked as slots or signals we use VisitDecl
    }

    // Here the user is connecting to a const method, which isn't marked as slot or signal and returns non-void
    // Looks like a getter!

    emitWarning(stmt, slot->getQualifiedNameAsString() + " is not a slot, and is possibly a getter");
}

void ConstSignalOrSlot::VisitDecl(Decl *decl)
{
    auto *method = dyn_cast<CXXMethodDecl>(decl);
    if (!method || !method->isConst()) {
        return;
    }

    const AccessSpecifierManager *a = m_context->accessSpecifierManager;
    if (!a) {
        return;
    }

    if (method->isThisDeclarationADefinition() && !method->hasInlineBody()) { // Don't warn twice
        return;
    }

    const CXXRecordDecl *record = method->getParent();
    if (clazy::derivesFrom(record, "QDBusAbstractInterface")) {
        return;
    }

    QtAccessSpecifierType specifierType = a->qtAccessSpecifierType(method);

    const bool isSlot = specifierType == QtAccessSpecifier_Slot;
    const bool isSignal = specifierType == QtAccessSpecifier_Signal;

    if (!isSlot && !isSignal) {
        return;
    }

    if (a->isScriptable(method)) {
        return;
    }

    if (isSlot && !method->getReturnType()->isVoidType()) {
        emitWarning(decl, "getter " + method->getQualifiedNameAsString() + " possibly mismarked as a slot");
    } else if (isSignal) {
        emitWarning(decl, "signal " + method->getQualifiedNameAsString() + " shouldn't be const");
    }
}
