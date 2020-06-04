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

#include "qrequiredresult-candidates.h"
#include "Utils.h"
#include "HierarchyUtils.h"
#include "QtUtils.h"
#include "TypeUtils.h"

#include <clang/AST/AST.h>

using namespace clang;
using namespace std;


QRequiredResultCandidates::QRequiredResultCandidates(const std::string &name, ClazyContext *context)
    : CheckBase(name, context)
{
}

void QRequiredResultCandidates::VisitDecl(clang::Decl *decl)
{
    auto method = dyn_cast<CXXMethodDecl>(decl);
    if (!method || !method->isConst())
        return;

    if (method->isThisDeclarationADefinition() && !method->hasInlineBody()) // Don't warn twice
        return;

    if (clazy::hasUnusedResultAttr(method)) // Also catches nodiscard
        return;

    if (method->getAccess() == AS_private) // We're only interested on our public API
        return;

    QualType qt = method->getReturnType();
    CXXRecordDecl* returnClass = qt->getAsCXXRecordDecl();
    returnClass = returnClass ? returnClass->getCanonicalDecl() : nullptr;
    if (!returnClass)
        return;

    CXXRecordDecl* classDecl = method->getParent();
    classDecl= classDecl ? classDecl->getCanonicalDecl() : nullptr;

    if (classDecl->getAccess() == AS_private) // A nested private class. We're only interested on our public API
        return;


    if (returnClass == classDecl) {
        const std::string methodName = static_cast<std::string>(clazy::name(method));
        if (methodName.empty()) // fixes assert
            return;

        if (clazy::startsWith(methodName, "to") || clazy::startsWith(methodName, "operator") || !clazy::endsWith(methodName, "ed"))
            return;

        emitWarning(decl, "Add Q_REQUIRED_RESULT to " + method->getQualifiedNameAsString() + "()");
    }
}
