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
#include "QtUtils.h"
#include "AccessSpecifierManager.h"
#include "ClazyContext.h"
#include "FunctionUtils.h"
#include "StringUtils.h"

#include <clang/AST/DeclCXX.h>
#include <clang/Basic/LLVM.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Casting.h>

namespace clang {
class Decl;
}  // namespace clang

using namespace clang;
using namespace std;


OverriddenSignal::OverriddenSignal(const std::string &name, ClazyContext *context)
    : CheckBase(name, context, Option_CanIgnoreIncludes)
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
    CXXRecordDecl *baseClass = clazy::getQObjectBaseClass(record);
    if (!baseClass)
        return;

    const bool methodIsSignal = accessSpecifierManager->qtAccessSpecifierType(method) == QtAccessSpecifier_Signal;
    const StringRef methodName = clazy::name(method);

    std::string warningMsg;
    while (baseClass) {
        for (auto baseMethod : baseClass->methods()) {
            if (clazy::name(baseMethod) == methodName) {

                if (!clazy::parametersMatch(method, baseMethod)) // overloading is permitted.
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

        baseClass = clazy::getQObjectBaseClass(baseClass);
    }
}
