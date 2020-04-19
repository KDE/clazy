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

#include "overloaded-signal.h"
#include "Utils.h"
#include "HierarchyUtils.h"
#include "QtUtils.h"
#include "TypeUtils.h"
#include "AccessSpecifierManager.h"

#include <ClazyContext.h>
#include <clang/AST/AST.h>

using namespace clang;
using namespace std;


OverloadedSignal::OverloadedSignal(const std::string &name, ClazyContext *context)
    : CheckBase(name, context)
{
    context->enableAccessSpecifierManager();
}

void OverloadedSignal::VisitDecl(clang::Decl *decl)
{
    AccessSpecifierManager *accessSpecifierManager = m_context->accessSpecifierManager;
    auto method = dyn_cast<CXXMethodDecl>(decl);
    if (!accessSpecifierManager || !method)
        return;

    if (method->isThisDeclarationADefinition() && !method->hasInlineBody())
        return;

    CXXRecordDecl *record = method->getParent();

    const bool methodIsSignal = accessSpecifierManager->qtAccessSpecifierType(method) == QtAccessSpecifier_Signal;
    if (!methodIsSignal)
        return;


    const StringRef methodName = clazy::name(method);
    CXXRecordDecl *p = record; // baseClass starts at record so we check overloaded methods there
    while (p) {
        for (auto m : p->methods()) {
            if (clazy::name(m) == methodName) {
                if (!clazy::parametersMatch(m, method)) {
                    if (p == record) {
                        emitWarning(decl, "signal " + methodName.str() + " is overloaded");
                        continue; // No point in spitting more warnings for the same signal
                    } else {
                        emitWarning(decl, "signal " + methodName.str() + " is overloaded (with " + clazy::getLocStart(p).printToString(sm()) + ")");
                    }
                }
            }
        }

        p = clazy::getQObjectBaseClass(p);
    }
}
