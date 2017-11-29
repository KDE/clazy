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

#include "overridden-signal.h"
#include "Utils.h"
#include "HierarchyUtils.h"
#include "QtUtils.h"
#include "TypeUtils.h"
#include "checkmanager.h"
#include "AccessSpecifierManager.h"
#include "ClazyContext.h"
#include "FunctionUtils.h"

#include <clang/AST/AST.h>

using namespace clang;
using namespace std;


OverriddenSignal::OverriddenSignal(const std::string &name, ClazyContext *context)
    : CheckBase(name, context)
{
    context->enableAccessSpecifierManager();
}

void OverriddenSignal::VisitDecl(clang::Decl *decl)
{
    AccessSpecifierManager *accessSpecifierManager = m_context->accessSpecifierManager;
    auto method = dyn_cast<CXXMethodDecl>(decl);
    if (!accessSpecifierManager || !method)
        return;

    if (method->isThisDeclarationADefinition() && !method->hasInlineBody())
        return;

    CXXRecordDecl *record = method->getParent();
    if (!QtUtils::isQObject(record))
        return;

    CXXRecordDecl *baseClass = QtUtils::getQObjectBaseClass(record);
    if (!baseClass)
        return;

    const bool methodIsSignal = accessSpecifierManager->qtAccessSpecifierType(method) == QtAccessSpecifier_Signal;
    const std::string methodName = method->getNameAsString();

    std::string warningMsg;
    while (baseClass) {
        for (auto baseMethod : baseClass->methods()) {
            if (baseMethod->getNameAsString() == methodName) {

                if (!FunctionUtils::parametersMatch(method, baseMethod)) // overloading is permitted.
                    continue;

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

        baseClass = QtUtils::getQObjectBaseClass(baseClass);
    }
}


REGISTER_CHECK("overridden-signal", OverriddenSignal, CheckLevel1)
