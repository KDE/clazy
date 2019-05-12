/*
  This file is part of the clazy static checker.

    Copyright (C) 2017 Sergio Martins <smartins@kde.org>

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

#include "const-signal-or-slot.h"
#include "QtUtils.h"
#include "TypeUtils.h"
#include "ClazyContext.h"
#include "AccessSpecifierManager.h"

#include <clang/AST/DeclCXX.h>
#include <clang/AST/Expr.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/Type.h>
#include <clang/Basic/LLVM.h>
#include <llvm/Support/Casting.h>

namespace clang {
class Decl;
class FunctionDecl;
}  // namespace clang

using namespace clang;
using namespace std;


ConstSignalOrSlot::ConstSignalOrSlot(const std::string &name, ClazyContext *context)
    : CheckBase(name, context, Option_CanIgnoreIncludes)
{
    context->enableAccessSpecifierManager();
}

void ConstSignalOrSlot::VisitStmt(clang::Stmt *stmt)
{
    auto call = dyn_cast<CallExpr>(stmt);
    AccessSpecifierManager *accessSpecifierManager = m_context->accessSpecifierManager;
    if (!call || !accessSpecifierManager)
        return;

    FunctionDecl *func = call->getDirectCallee();
    if (!clazy::isConnect(func) || !clazy::connectHasPMFStyle(func))
        return;

    CXXMethodDecl *slot =  clazy::receiverMethodForConnect(call);
    if (!slot || !slot->isConst() || slot->getReturnType()->isVoidType()) // const and returning void must do something, so not a getter
        return;

    QtAccessSpecifierType specifierType = accessSpecifierManager->qtAccessSpecifierType(slot);
    if (specifierType == QtAccessSpecifier_Slot || specifierType == QtAccessSpecifier_Signal)
        return; // For stuff explicitly marked as slots or signals we use VisitDecl


    // Here the user is connecting to a const method, which isn't marked as slot or signal and returns non-void
    // Looks like a getter!

    emitWarning(stmt, slot->getQualifiedNameAsString() + " is not a slot, and is possibly a getter");
}

void ConstSignalOrSlot::VisitDecl(Decl *decl)
{
    auto method = dyn_cast<CXXMethodDecl>(decl);
    if (!method || !method->isConst())
        return;

    AccessSpecifierManager *a = m_context->accessSpecifierManager;
    if (!a)
        return;

    if (method->isThisDeclarationADefinition() && !method->hasInlineBody()) // Don't warn twice
        return;

    CXXRecordDecl *record = method->getParent();
    if (clazy::derivesFrom(record, "QDBusAbstractInterface"))
        return;

    QtAccessSpecifierType specifierType = a->qtAccessSpecifierType(method);

    const bool isSlot = specifierType == QtAccessSpecifier_Slot;
    const bool isSignal = specifierType == QtAccessSpecifier_Signal;

    if (!isSlot && !isSignal)
        return;

    if (a->isScriptable(method))
        return;

    if (isSlot && !method->getReturnType()->isVoidType()) {
        emitWarning(decl, "getter " + method->getQualifiedNameAsString() + " possibly mismarked as a slot");
    } else if (isSignal) {
        emitWarning(decl, "signal " + method->getQualifiedNameAsString() + " shouldn't be const");
    }
}
