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
#include "QtUtils.h"
#include "TypeUtils.h"
#include "SourceCompatibilityHelpers.h"
#include "StringUtils.h"

#include <clang/AST/DeclBase.h>
#include <clang/AST/DeclCXX.h>
#include <clang/Basic/LLVM.h>
#include <clang/Basic/SourceManager.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Casting.h>

class ClazyContext;

using namespace clang;
using namespace std;


CtorMissingParentArgument::CtorMissingParentArgument(const std::string &name, ClazyContext *context)
    : CheckBase(name, context)
{
}

static string expectedParentTypeFor(CXXRecordDecl *decl)
{
    if (clazy::derivesFrom(decl, "QWidget")) {
        return "QWidget";
    } else if (clazy::derivesFrom(decl, "QQuickItem")) {
        return "QQuickItem";
    } else if (clazy::derivesFrom(decl, "Qt3DCore::QEntity")) {
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

    if (record->hasInheritedConstructor()) {
        // When doing using QObject::QObject you inherit the ctors from QObject, so don't warn.
        // Would be nicer to check if the using directives really refer to QObject::QObject and not to
        // Some other non-object class, but I can't find a way to get to ConstructorUsingShadowDecl from the CxxRecordDecl
        // so we might miss some true-positives
        return;
    }

    const bool hasCtors = record->ctor_begin() != record->ctor_end();
    if (!hasCtors)
        return;

    const string parentType = expectedParentTypeFor(record);
    int numCtors = 0;
    const bool hasQObjectParam = clazy::recordHasCtorWithParam(record, parentType, /*by-ref*/ ok, /*by-ref*/ numCtors);
    if (!ok)
        return;

    if (numCtors > 0 && !hasQObjectParam) {
        clang::CXXRecordDecl *baseClass = clazy::getQObjectBaseClass(record);
        const bool baseHasQObjectParam = clazy::recordHasCtorWithParam(baseClass, parentType, /*by-ref*/ ok, /*by-ref*/ numCtors);
        if (ok && !baseHasQObjectParam && sm().isInSystemHeader(clazy::getLocStart(baseClass))) {
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
