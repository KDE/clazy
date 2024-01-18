/*
    SPDX-FileCopyrightText: 2019 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "overloaded-signal.h"
#include "AccessSpecifierManager.h"
#include "FunctionUtils.h"
#include "QtUtils.h"
#include "TypeUtils.h"
#include "Utils.h"

#include <ClazyContext.h>
#include <clang/AST/AST.h>

using namespace clang;

OverloadedSignal::OverloadedSignal(const std::string &name, ClazyContext *context)
    : CheckBase(name, context)
{
    context->enableAccessSpecifierManager();
}

void OverloadedSignal::VisitDecl(clang::Decl *decl)
{
    AccessSpecifierManager *accessSpecifierManager = m_context->accessSpecifierManager;
    auto *method = dyn_cast<CXXMethodDecl>(decl);
    if (!accessSpecifierManager || !method) {
        return;
    }

    if (method->isThisDeclarationADefinition() && !method->hasInlineBody()) {
        return;
    }

    CXXRecordDecl *record = method->getParent();

    const bool methodIsSignal = accessSpecifierManager->qtAccessSpecifierType(method) == QtAccessSpecifier_Signal;
    if (!methodIsSignal) {
        return;
    }

    const StringRef methodName = clazy::name(method);
    CXXRecordDecl *p = record; // baseClass starts at record so we check overloaded methods there
    while (p) {
        for (auto *m : p->methods()) {
            if (clazy::name(m) == methodName) {
                if (!clazy::parametersMatch(m, method)) {
                    if (p == record) {
                        emitWarning(decl, "signal " + methodName.str() + " is overloaded");
                        continue; // No point in spitting more warnings for the same signal
                    }
                    emitWarning(decl, "signal " + methodName.str() + " is overloaded (with " + clazy::getLocStart(p).printToString(sm()) + ")");
                }
            }
        }

        p = clazy::getQObjectBaseClass(p);
    }
}
