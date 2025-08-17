/*
    SPDX-FileCopyrightText: 2015 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "rule-of-three.h"
#include "MacroUtils.h"
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

using namespace clang;

RuleOfThree::RuleOfThree(const std::string &name, Options options)
    : RuleOfBase(name, options)
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

    if (shouldIgnoreFile(decl->getBeginLoc())) {
        return;
    }

    const SourceLocation recordStart = record->getBeginLoc();
    if (recordStart.isMacroID()) {
        if (clazy::isInMacro(astContext(), recordStart, "Q_GLOBAL_STATIC_INTERNAL")) {
            return;
        }
    }

    CXXConstructorDecl *copyCtor = Utils::copyCtor(record);
    CXXMethodDecl *copyAssign = Utils::copyAssign(record);
    CXXDestructorDecl *destructor = nullptr;
    // Getting the destructor using record->getDestructor() does not work for later clang versions, e.g. clang 16
    for (auto *decl : record->decls()) {
        if (auto *destructorDecl = dyn_cast<CXXDestructorDecl>(decl)) {
            destructor = destructorDecl;
            break;
        }
    }
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
        hasList.emplace_back("dtor");
    } else {
        missingList.emplace_back("dtor");
    }

    if (hasUserCopyCtor) {
        hasList.emplace_back("copy-ctor");
    } else {
        missingList.emplace_back("copy-ctor");
    }

    if (hasUserCopyAssign) {
        hasList.emplace_back("copy-assignment");
    } else {
        missingList.emplace_back("copy-assignment");
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

    emitWarning(decl->getBeginLoc(), msg);
}
