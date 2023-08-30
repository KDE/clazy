/*
  This file is part of the clazy static checker.

    SPDX-FileCopyrightText: 2018 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "qrequiredresult-candidates.h"
#include "HierarchyUtils.h"
#include "QtUtils.h"
#include "TypeUtils.h"
#include "Utils.h"

#include <clang/AST/AST.h>

using namespace clang;

QRequiredResultCandidates::QRequiredResultCandidates(const std::string &name, ClazyContext *context)
    : CheckBase(name, context)
{
}

void QRequiredResultCandidates::VisitDecl(clang::Decl *decl)
{
    auto *method = dyn_cast<CXXMethodDecl>(decl);
    if (!method || !method->isConst()) {
        return;
    }

    if (method->isThisDeclarationADefinition() && !method->hasInlineBody()) { // Don't warn twice
        return;
    }

    if (clazy::hasUnusedResultAttr(method)) { // Also catches nodiscard
        return;
    }

    if (method->getAccess() == AS_private) { // We're only interested on our public API
        return;
    }

    QualType qt = method->getReturnType();
    CXXRecordDecl *returnClass = qt->getAsCXXRecordDecl();
    returnClass = returnClass ? returnClass->getCanonicalDecl() : nullptr;
    if (!returnClass) {
        return;
    }

    CXXRecordDecl *classDecl = method->getParent();
    classDecl = classDecl ? classDecl->getCanonicalDecl() : nullptr;

    if (classDecl->getAccess() == AS_private) { // A nested private class. We're only interested on our public API
        return;
    }

    if (returnClass == classDecl) {
        const std::string methodName = static_cast<std::string>(clazy::name(method));
        if (methodName.empty()) { // fixes assert
            return;
        }

        if (clazy::startsWith(methodName, "to") || clazy::startsWith(methodName, "operator") || !clazy::endsWith(methodName, "ed")) {
            return;
        }

        emitWarning(decl, "Add Q_REQUIRED_RESULT to " + method->getQualifiedNameAsString() + "()");
    }
}
