/*
    SPDX-FileCopyrightText: 2019 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "signal-with-return-value.h"
#include "AccessSpecifierManager.h"
#include "ClazyContext.h"
#include "QtUtils.h"
#include "TypeUtils.h"
#include "Utils.h"

#include <clang/AST/AST.h>

using namespace clang;

void SignalWithReturnValue::VisitDecl(clang::Decl *decl)
{
    const AccessSpecifierManager *accessSpecifierManager = m_context->accessSpecifierManager;
    auto *method = dyn_cast<CXXMethodDecl>(decl);
    if (!accessSpecifierManager || !method) {
        return;
    }

    if (method->isThisDeclarationADefinition() && !method->hasInlineBody()) {
        return;
    }

    const bool methodIsSignal = accessSpecifierManager->qtAccessSpecifierType(method) == QtAccessSpecifier_Signal;
    if (!methodIsSignal || accessSpecifierManager->isScriptable(method)) {
        return;
    }

    if (!method->getReturnType()->isVoidType()) {
        emitWarning(decl,
                    std::string(clazy::name(method))
                        + "() should return void. For a clean design signals shouldn't assume a single slot are connected to them.");
    }

    for (auto *param : method->parameters()) {
        QualType qt = param->getType();
        if (qt->isReferenceType() && !qt->getPointeeType().isConstQualified()) {
            emitWarning(decl,
                        std::string(clazy::name(method))
                            + "() shouldn't receive parameters by ref. For a clean design signals shouldn't assume a single slot are connected to them.");
        }
    }
}
