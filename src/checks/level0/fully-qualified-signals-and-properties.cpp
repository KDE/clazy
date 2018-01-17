/*
  This file is part of the clazy static checker.

  Copyright (C) 2018 Sergio Martins <smartins@kde.org>

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

#include "fully-qualified-signals-and-properties.h"
#include "Utils.h"
#include "HierarchyUtils.h"
#include "QtUtils.h"
#include "TypeUtils.h"
#include "ClazyContext.h"
#include "AccessSpecifierManager.h"

#include <clang/AST/AST.h>

using namespace clang;
using namespace std;


FullyQualifiedSignalsAndProperties::FullyQualifiedSignalsAndProperties(const std::string &name, ClazyContext *context)
    : CheckBase(name, context)
{
    context->enableAccessSpecifierManager();
}

void FullyQualifiedSignalsAndProperties::VisitDecl(clang::Decl *decl)
{
    auto method = dyn_cast<CXXMethodDecl>(decl);
    if (!method)
        return;

    if (method->isThisDeclarationADefinition() && !method->hasInlineBody())
        return;

    AccessSpecifierManager *accessSpecifierManager = m_context->accessSpecifierManager;
    if (!accessSpecifierManager)
        return;

    QtAccessSpecifierType qst = accessSpecifierManager->qtAccessSpecifierType(method);
    if (qst != QtAccessSpecifier_Signal)
        return;

    for (auto param : method->parameters()) {
        QualType t = TypeUtils::pointeeQualType(param->getType());
        if (!t.isNull()) {
            std::string typeName = clazy::name(t, lo(), /*asWritten=*/ true);
            if (typeName == "QPrivateSignal")
                continue;

            std::string qualifiedTypeName = clazy::name(t, lo(), /*asWritten=*/ false);

            if (typeName != qualifiedTypeName) {
                emitWarning(method, "signal arguments need to be fully-qualified (" + qualifiedTypeName + " instead of " + typeName + ")");
            }
        }
    }
}
