/*
  This file is part of the clazy static checker.

  Copyright (C) 2016 Sergio Martins <smartins@kde.org>

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

#include "ctor-missing-parent-argument.h"
#include "Utils.h"
#include "HierarchyUtils.h"
#include "QtUtils.h"
#include "TypeUtils.h"

#include <clang/AST/AST.h>

using namespace clang;
using namespace std;


CtorMissingParentArgument::CtorMissingParentArgument(const std::string &name, ClazyContext *context)
    : CheckBase(name, context)
{
}

static string expectedParentTypeFor(CXXRecordDecl *decl)
{
    if (TypeUtils::derivesFrom(decl, "QWidget")) {
        return "QWidget";
    } else if (TypeUtils::derivesFrom(decl, "QQuickItem")) {
        return "QQuickItem";
    } else if (TypeUtils::derivesFrom(decl, "Qt3DCore::QEntity")) {
        return "Qt3DCore::QNode";
    }

    return "QObject";
}

void CtorMissingParentArgument::VisitDecl(Decl *decl)
{
    auto record = dyn_cast<CXXRecordDecl>(decl);
    bool ok = false;

    if (!clazy::isQObject(record))
        return;

    const bool hasCtors = record->ctor_begin() != record->ctor_end();
    if (!hasCtors)
        return;

    const string parentType = expectedParentTypeFor(record);
    int numCtors = 0;
    const bool hasQObjectParam = clazy::recordHasCtorWithParam(record, parentType, /*by-ref*/ok, /*by-ref*/numCtors);
    if (!ok)
        return;

    if (numCtors > 0 && !hasQObjectParam) {
        clang::CXXRecordDecl *baseClass = clazy::getQObjectBaseClass(record);
        const bool baseHasQObjectParam = clazy::recordHasCtorWithParam(baseClass, parentType, /*by-ref*/ok, /*by-ref*/numCtors);
        if (ok && !baseHasQObjectParam && sm().isInSystemHeader(baseClass->getLocStart())) {
            // If the base class ctors don't accept QObject, and it's declared in a system header don't warn
            return;
        }

        if (clazy::name(baseClass) == "QCoreApplication")
            return;

        emitWarning(decl, record->getQualifiedNameAsString() +
                    string(" should take ") +
                    parentType + string(" parent argument in CTOR"));
    }
}
