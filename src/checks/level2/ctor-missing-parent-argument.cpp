/*
    SPDX-FileCopyrightText: 2016 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "ctor-missing-parent-argument.h"
#include "QtUtils.h"
#include "StringUtils.h"
#include "TypeUtils.h"

#include <clang/AST/DeclBase.h>
#include <clang/AST/DeclCXX.h>
#include <clang/Basic/LLVM.h>
#include <clang/Basic/SourceManager.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Casting.h>

using namespace clang;

CtorMissingParentArgument::CtorMissingParentArgument(const std::string &name, ClazyContext *context)
    : CheckBase(name, context)
{
}

static std::string expectedParentTypeFor(CXXRecordDecl *decl)
{
    if (clazy::derivesFrom(decl, "QWidget")) {
        return "QWidget";
    }
    if (clazy::derivesFrom(decl, "QQuickItem")) {
        return "QQuickItem";
    } else if (clazy::derivesFrom(decl, "Qt3DCore::QEntity")) {
        return "Qt3DCore::QNode";
    }

    return "QObject";
}

void CtorMissingParentArgument::VisitDecl(Decl *decl)
{
    auto *record = dyn_cast<CXXRecordDecl>(decl);
    bool ok = false;

    if (!clazy::isQObject(record)) {
        return;
    }

    if (record->hasInheritedConstructor()) {
        // When doing using QObject::QObject you inherit the ctors from QObject, so don't warn.
        // Would be nicer to check if the using directives really refer to QObject::QObject and not to
        // Some other non-object class, but I can't find a way to get to ConstructorUsingShadowDecl from the CxxRecordDecl
        // so we might miss some true-positives
        return;
    }

    const bool hasCtors = record->ctor_begin() != record->ctor_end();
    if (!hasCtors) {
        return;
    }

    const std::string parentType = expectedParentTypeFor(record);
    int numCtors = 0;
    const bool hasQObjectParam = clazy::recordHasCtorWithParam(record, parentType, /*by-ref*/ ok, /*by-ref*/ numCtors);
    if (!ok) {
        return;
    }

    if (numCtors > 0 && !hasQObjectParam) {
        clang::CXXRecordDecl *baseClass = clazy::getQObjectBaseClass(record);
        const bool baseHasQObjectParam = clazy::recordHasCtorWithParam(baseClass, parentType, /*by-ref*/ ok, /*by-ref*/ numCtors);
        if (ok && !baseHasQObjectParam && sm().isInSystemHeader(baseClass->getBeginLoc())) {
            // If the base class ctors don't accept QObject, and it's declared in a system header don't warn
            return;
        }

        if (clazy::name(baseClass) == "QCoreApplication") {
            return;
        }

        emitWarning(decl, record->getQualifiedNameAsString() + std::string(" should take ") + parentType + std::string(" parent argument in CTOR"));
    }
}
