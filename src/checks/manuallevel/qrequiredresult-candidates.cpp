/*
    SPDX-FileCopyrightText: 2018 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "qrequiredresult-candidates.h"
#include "QtUtils.h"
#include "TypeUtils.h"
#include "Utils.h"

#include <clang/AST/AST.h>

using namespace clang;

static bool hasUnusedResultAttr(clang::FunctionDecl *func)
{
    auto RetType = func->getReturnType();
    if (const auto *Ret = RetType->getAsRecordDecl()) {
        if (const auto *R = Ret->getAttr<clang::WarnUnusedResultAttr>()) {
            return R != nullptr;
        }
    } else if (const auto *ET = RetType->getAs<clang::EnumType>()) {
        if (const clang::EnumDecl *ED = ET->getDecl()) {
            if (const auto *R = ED->getAttr<clang::WarnUnusedResultAttr>()) {
                return R != nullptr;
            }
        }
    }
    return func->getAttr<clang::WarnUnusedResultAttr>() != nullptr;
}

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

    if (hasUnusedResultAttr(method)) { // Also catches nodiscard
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
