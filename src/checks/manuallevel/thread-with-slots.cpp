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

#include "thread-with-slots.h"
#include "HierarchyUtils.h"
#include "QtUtils.h"
#include "TypeUtils.h"
#include "ClazyContext.h"
#include "AccessSpecifierManager.h"

#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/Expr.h>
#include <clang/AST/Stmt.h>
#include <clang/Basic/LLVM.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Casting.h>

#include <vector>

namespace clang {
class Decl;
}  // namespace clang

using namespace clang;
using namespace std;


static bool hasMutexes(Stmt *body)
{
    auto declrefs = clazy::getStatements<DeclRefExpr>(body);
    for (auto declref : declrefs) {
        ValueDecl *valueDecl = declref->getDecl();
        if (CXXRecordDecl *record = clazy::typeAsRecord(valueDecl->getType())) {
            if (clazy::name(record) == "QMutex" || clazy::name(record) == "QBasicMutex") {
                return true;
            }
        }
    }

    return false;
}

ThreadWithSlots::ThreadWithSlots(const std::string &name, ClazyContext *context)
    : CheckBase(name, context)
{
    context->enableAccessSpecifierManager();
}

void ThreadWithSlots::VisitStmt(clang::Stmt *stmt)
{
    // Here we catch slots not marked as slots, we warn when the connect is made

    auto callExpr = dyn_cast<CallExpr>(stmt);
    if (!callExpr || !m_context->accessSpecifierManager)
        return;

    FunctionDecl *connectFunc = callExpr->getDirectCallee();
    if (!clazy::isConnect(connectFunc))
        return;

    CXXMethodDecl *slot =  clazy::receiverMethodForConnect(callExpr);
    if (!slot || !clazy::derivesFrom(slot->getParent(), "QThread"))
        return;

    if (clazy::name(slot->getParent()) == "QThread") // The slots in QThread are thread safe, we're only worried about derived classes
        return;

    QtAccessSpecifierType specifierType = m_context->accessSpecifierManager->qtAccessSpecifierType(slot);
    if (specifierType == QtAccessSpecifier_Slot || specifierType == QtAccessSpecifier_Signal)
        return; // For stuff explicitly marked as slots or signals we use VisitDecl

    emitWarning(slot, "Slot " + slot->getQualifiedNameAsString() + " might not run in the expected thread");
}

void ThreadWithSlots::VisitDecl(Decl *decl)
{
    // Here we catch slots marked as such, and warn when they are declared

    auto method = dyn_cast<CXXMethodDecl>(decl);
    if (!method || !m_context->accessSpecifierManager || !method->isThisDeclarationADefinition() || !method->hasBody()
        || !clazy::derivesFrom(method->getParent(), "QThread"))
        return;

    // The slots in QThread are thread safe, we're only worried about derived classes:
    if (clazy::name(method->getParent()) == "QThread")
        return;

    // We're only interested in slots:
    if (m_context->accessSpecifierManager->qtAccessSpecifierType(method) != QtAccessSpecifier_Slot)
        return;

    // Look for a mutex, or mutex locker, to avoid some false-positives
    Stmt *body = method->getBody();
    if (hasMutexes(body))
        return;

    // If we use member mutexes, let's not warn either
    bool accessesNonMutexMember = false;
    auto memberexprs = clazy::getStatements<MemberExpr>(body);
    for (auto memberexpr : memberexprs) {
        ValueDecl *valueDecl = memberexpr->getMemberDecl();
        if (CXXRecordDecl *record = clazy::typeAsRecord(valueDecl->getType())) {
            if (clazy::name(record) == "QMutex" || clazy::name(record) == "QBasicMutex") {
                return;
            }
        }
        accessesNonMutexMember = true;
    }

    if (!accessesNonMutexMember)
        return;

    emitWarning(method, "Slot " + method->getQualifiedNameAsString() + " might not run in the expected thread");
}


