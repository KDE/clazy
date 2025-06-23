/*
    SPDX-FileCopyrightText: 2017 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "overridden-signal.h"
#include "AccessSpecifierManager.h"
#include "ClazyContext.h"
#include "FunctionUtils.h"
#include "QtUtils.h"
#include "StringUtils.h"

#include <clang/AST/DeclCXX.h>
#include <clang/Basic/LLVM.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Casting.h>

using namespace clang;

OverriddenSignal::OverriddenSignal(const std::string &name)
    : CheckBase(name, Option_CanIgnoreIncludes)
{
}

void OverriddenSignal::VisitDecl(clang::Decl *decl)
{
    const AccessSpecifierManager *accessSpecifierManager = m_context->accessSpecifierManager;
    auto *method = dyn_cast<CXXMethodDecl>(decl);
    if (!accessSpecifierManager || !method) {
        return;
    }

    if (method->isThisDeclarationADefinition() && !method->hasInlineBody()) {
        return;
    }

    CXXRecordDecl *record = method->getParent();
    CXXRecordDecl *baseClass = clazy::getQObjectBaseClass(record);
    if (!baseClass) {
        return;
    }

    const bool methodIsSignal = accessSpecifierManager->qtAccessSpecifierType(method) == QtAccessSpecifier_Signal;
    const StringRef methodName = clazy::name(method);

    std::string warningMsg;
    while (baseClass) {
        for (auto *baseMethod : baseClass->methods()) {
            if (clazy::name(baseMethod) == methodName) {
                if (!clazy::parametersMatch(method, baseMethod)) { // overloading is permitted.
                    continue;
                }

                const bool baseMethodIsSignal = accessSpecifierManager->qtAccessSpecifierType(baseMethod) == QtAccessSpecifier_Signal;

                if (methodIsSignal && baseMethodIsSignal) {
                    warningMsg = "Overriding signal with signal: " + method->getQualifiedNameAsString();
                } else if (methodIsSignal && !baseMethodIsSignal) {
                    warningMsg = "Overriding non-signal with signal: " + method->getQualifiedNameAsString();
                } else if (!methodIsSignal && baseMethodIsSignal) {
                    warningMsg = "Overriding signal with non-signal: " + method->getQualifiedNameAsString();
                }

                if (!warningMsg.empty()) {
                    emitWarning(decl, warningMsg);
                    return;
                }
            }
        }

        baseClass = clazy::getQObjectBaseClass(baseClass);
    }
}
