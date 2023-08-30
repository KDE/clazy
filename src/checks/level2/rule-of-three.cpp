/*
    This file is part of the clazy static checker.

    SPDX-FileCopyrightText: 2015 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "rule-of-three.h"
#include "MacroUtils.h"
#include "SourceCompatibilityHelpers.h"
#include "TypeUtils.h"
#include "Utils.h"
#include "clazy_stl.h"

#include <clang/AST/DeclBase.h>
#include <clang/AST/DeclCXX.h>
#include <clang/Basic/LLVM.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Basic/Specifiers.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Casting.h>

#include <vector>

class ClazyContext;

using namespace clang;

RuleOfThree::RuleOfThree(const std::string &name, ClazyContext *context)
    : RuleOfBase(name, context)
{
    m_filesToIgnore = {"qrc_"};
}

void RuleOfThree::VisitDecl(clang::Decl *decl)
{
    auto *record = dyn_cast<CXXRecordDecl>(decl);
    if (!record || isBlacklisted(record) || !record->hasDefinition() || record->isPolymorphic()) {
        return;
    }

    // fwd decl is not interesting
    if (record != record->getDefinition()) {
        return;
    }

    if (shouldIgnoreFile(clazy::getLocStart(decl))) {
        return;
    }

    const SourceLocation recordStart = clazy::getLocStart(record);
    if (recordStart.isMacroID()) {
        if (clazy::isInMacro(&m_astContext, recordStart, "Q_GLOBAL_STATIC_INTERNAL")) {
            return;
        }
    }

    CXXConstructorDecl *copyCtor = Utils::copyCtor(record);
    CXXMethodDecl *copyAssign = Utils::copyAssign(record);
    CXXDestructorDecl *destructor = record->getDestructor();
    const bool dtorDefaultedByUser = destructor && destructor->isDefaulted() && !destructor->isImplicit();

    const bool hasUserCopyCtor = copyCtor && copyCtor->isUserProvided();
    const bool hasUserCopyAssign = copyAssign && copyAssign->isUserProvided();
    const bool hasUserDtor = destructor && destructor->isUserProvided();

    const bool copyCtorIsDeleted = copyCtor && copyCtor->isDeleted();
    const bool copyAssignIsDeleted = copyAssign && copyAssign->isDeleted();

    bool hasImplicitDeletedCopy = false;
    if (!copyCtor || !copyAssign) {
        for (auto *f : record->fields()) {
            QualType qt = f->getType();
            if (qt.isConstQualified() || qt->isRValueReferenceType()) {
                hasImplicitDeletedCopy = true;
                break;
            }
        }
    }

    if (hasUserDtor && (copyCtorIsDeleted || copyAssignIsDeleted || hasImplicitDeletedCopy)) {
        // One of the copy methods was explicitely deleted, it's safe.
        // The case we want to catch is when one is user-written and the other is
        // compiler-generated.
        return;
    }

    const int numImplemented = hasUserCopyCtor + hasUserCopyAssign + hasUserDtor;
    if (numImplemented == 0 || numImplemented == 3) { // Rule of 3 respected
        return;
    }

    std::vector<StringRef> hasList;
    std::vector<StringRef> missingList;
    if (hasUserDtor) {
        hasList.push_back("dtor");
    } else {
        missingList.push_back("dtor");
    }

    if (hasUserCopyCtor) {
        hasList.push_back("copy-ctor");
    } else {
        missingList.push_back("copy-ctor");
    }

    if (hasUserCopyAssign) {
        hasList.push_back("copy-assignment");
    } else {
        missingList.push_back("copy-assignment");
    }

    const int numNotImplemented = missingList.size();

    if (hasUserDtor && numImplemented == 1) {
        // Protected dtor is a way for a non-polymorphic base class avoid being deleted
        if (destructor->getAccess() == clang::AccessSpecifier::AS_protected) {
            return;
        }

        if (Utils::functionHasEmptyBody(destructor)) {
            // Lets reduce noise and allow the empty dtor. In theory we could warn, but there's no
            // hidden bug behind this dummy dtor.
            return;
        }
    }

    if (!hasUserDtor && (clazy::derivesFrom(record, "QSharedData") || dtorDefaultedByUser)) {
        return;
    }

    if (Utils::hasMember(record, "QSharedDataPointer")) {
        return; // These need boiler-plate copy ctor and dtor
    }

    const std::string className = record->getNameAsString();
    const std::string classQualifiedName = record->getQualifiedNameAsString();
    const std::string filename = static_cast<std::string>(sm().getFilename(recordStart));
    if (clazy::endsWith(className, "Private") && clazy::endsWithAny(filename, {".cpp", ".cxx", "_p.h"})) {
        return; // Lots of RAII classes fall into this category. And even Private (d-pointer) classes, warning in that case would just be noise
    }

    std::string msg = classQualifiedName + " has ";

    for (int i = 0; i < numImplemented; ++i) {
        msg += hasList[i];
        const bool isLast = i == numImplemented - 1;
        if (!isLast) {
            msg += ',';
        }
        msg += ' ';
    }

    msg += "but not ";
    for (int i = 0; i < numNotImplemented; ++i) {
        msg += missingList[i];
        const bool isLast = i == numNotImplemented - 1;
        if (!isLast) {
            msg += ", ";
        }
    }

    emitWarning(clazy::getLocStart(decl), msg);
}
