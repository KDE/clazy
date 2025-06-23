/*
    SPDX-FileCopyrightText: 2017 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "connect-by-name.h"
#include "AccessSpecifierManager.h"
#include "ClazyContext.h"
#include "clazy_stl.h"

#include <clang/AST/DeclCXX.h>
#include <clang/Basic/LLVM.h>
#include <llvm/Support/Casting.h>

#include <vector>

using namespace clang;

ConnectByName::ConnectByName(const std::string &name)
    : CheckBase(name)
{
}

void ConnectByName::VisitDecl(clang::Decl *decl)
{
    auto *record = dyn_cast<CXXRecordDecl>(decl);
    if (!record) {
        return;
    }

    const AccessSpecifierManager *accessSpecifierManager = m_context->accessSpecifierManager;
    if (!accessSpecifierManager) {
        return;
    }

    for (auto *method : record->methods()) {
        std::string name = method->getNameAsString();
        if (clazy::startsWith(name, "on_")) {
            QtAccessSpecifierType qst = accessSpecifierManager->qtAccessSpecifierType(method);
            if (qst == QtAccessSpecifier_Slot) {
                auto tokens = clazy::splitString(name, '_');
                if (tokens.size() == 3) {
                    emitWarning(method, "Slots named on_foo_bar are error prone");
                }
            }
        }
    }
}
