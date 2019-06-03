/*
  This file is part of the clazy static checker.

  Copyright (C) 2019 Sergio Martins <smartins@kde.org>

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

#include "signal-with-return-value.h"
#include "Utils.h"
#include "HierarchyUtils.h"
#include "QtUtils.h"
#include "TypeUtils.h"
#include "AccessSpecifierManager.h"
#include "ClazyContext.h"

#include <clang/AST/AST.h>

using namespace clang;
using namespace std;


SignalWithReturnValue::SignalWithReturnValue(const std::string &name, ClazyContext *context)
    : CheckBase(name, context)
{
    context->enableAccessSpecifierManager();
}

void SignalWithReturnValue::VisitDecl(clang::Decl *decl)
{
    AccessSpecifierManager *accessSpecifierManager = m_context->accessSpecifierManager;
    auto method = dyn_cast<CXXMethodDecl>(decl);
    if (!accessSpecifierManager || !method)
        return;

    if (method->isThisDeclarationADefinition() && !method->hasInlineBody())
        return;

    const bool methodIsSignal = accessSpecifierManager->qtAccessSpecifierType(method) == QtAccessSpecifier_Signal;
    if (!methodIsSignal || accessSpecifierManager->isScriptable(method))
        return;

    if (!method->getReturnType()->isVoidType())
        emitWarning(decl, std::string(clazy::name(method)) + "() should return void. For a clean design signals shouldn't assume a single slot are connected to them.");

    for (auto param : method->parameters()) {
        QualType qt = param->getType();
        if (qt->isReferenceType() && !qt->getPointeeType().isConstQualified()) {
            emitWarning(decl, std::string(clazy::name(method)) + "() shouldn't receive parameters by ref. For a clean design signals shouldn't assume a single slot are connected to them.");
        }
    }
}
