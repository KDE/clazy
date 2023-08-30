/*
  This file is part of the clazy static checker.

    SPDX-FileCopyrightText: 2017 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "virtual-signal.h"
#include "AccessSpecifierManager.h"
#include "ClazyContext.h"
#include "QtUtils.h"

#include <clang/AST/DeclCXX.h>
#include <clang/Basic/LLVM.h>
#include <llvm/Support/Casting.h>

namespace clang
{
class Decl;
} // namespace clang

using namespace clang;

VirtualSignal::VirtualSignal(const std::string &name, ClazyContext *context)
    : CheckBase(name, context)
{
    context->enableAccessSpecifierManager();
}

void VirtualSignal::VisitDecl(Decl *stmt)
{
    auto *method = dyn_cast<CXXMethodDecl>(stmt);
    if (!method || !method->isVirtual()) {
        return;
    }

    AccessSpecifierManager *accessSpecifierManager = m_context->accessSpecifierManager;
    if (!accessSpecifierManager) {
        return;
    }

    QtAccessSpecifierType qst = accessSpecifierManager->qtAccessSpecifierType(method);
    if (qst == QtAccessSpecifier_Signal) {
        for (const auto *m : method->overridden_methods()) {
            if (const auto *baseClass = m->getParent()) {
                if (!clazy::isQObject(baseClass)) {
                    // It's possible that the signal is overriding a method from a non-QObject base class
                    // if the derived class inherits both QObject and some other interface.
                    return;
                }
            }
        }

        emitWarning(method, "signal is virtual");
    }
}
