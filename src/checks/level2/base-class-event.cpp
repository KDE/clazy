/*
    SPDX-FileCopyrightText: 2016 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "base-class-event.h"
#include "HierarchyUtils.h"
#include "QtUtils.h"
#include "clazy_stl.h"

#include <clang/AST/DeclCXX.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/Stmt.h>
#include <clang/Basic/LLVM.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Casting.h>

#include <array>
#include <vector>

using namespace clang;

void BaseClassEvent::VisitDecl(Decl *decl)
{
    auto *method = dyn_cast<CXXMethodDecl>(decl);
    if (!method || !method->hasBody() || !method->isThisDeclarationADefinition()) {
        return;
    }

    const std::string methodName = method->getNameAsString();
    const bool isEvent = methodName == "event";
    const bool isEventFilter = isEvent ? false : methodName == "eventFilter";

    if (!isEvent && !isEventFilter) {
        return;
    }

    CXXRecordDecl *classDecl = method->getParent();
    if (!clazy::isQObject(classDecl, m_context->qtNamespace())) {
        return;
    }

    const std::string className = classDecl->getQualifiedNameAsString();
    if (clazy::contains(std::array<StringRef, 2>({"QObject", "QWidget"}), className)) {
        return;
    }

    CXXRecordDecl *baseClass = clazy::getQObjectBaseClass(classDecl, m_context->qtNamespace());
    const std::string baseClassName = baseClass ? baseClass->getQualifiedNameAsString() : std::string("BaseClass");

    if (isEventFilter && clazy::contains(std::array<StringRef, 2>({"QObject", "QWidget"}), baseClassName)) {
        // This is fine, QObject and QWidget eventFilter() don't do anything
        return;
    }

    Stmt *body = method->getBody();
    std::vector<ReturnStmt *> returns;
    clazy::getChilds<ReturnStmt>(body, /*by-ref*/ returns);
    for (ReturnStmt *returnStmt : returns) {
        Stmt *maybeBoolExpr = clazy::childAt(returnStmt, 0);
        if (!maybeBoolExpr) {
            continue;
        }
        auto *boolExpr = dyn_cast<CXXBoolLiteralExpr>(maybeBoolExpr);
        if (!boolExpr || boolExpr->getValue()) { // if getValue() is true that's a return true, which is fine
            continue;
        }

        emitWarning(returnStmt->getBeginLoc(), "Return " + baseClassName + "::" + methodName + "() instead of false");
    }
}
