/*
   This file is part of the clazy static checker.

  Copyright (C) 2015 Sergio Martins <smartins@kde.org>

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

#include "ruleofthree.h"
#include "Utils.h"
#include "MacroUtils.h"
#include "StringUtils.h"
#include "TypeUtils.h"

#include <clang/AST/AST.h>

using namespace clang;
using namespace std;

RuleOfThree::RuleOfThree(const std::string &name, ClazyContext *context)
    : RuleOfBase(name, context)
{
    m_filesToIgnore = { "qrc_" };
}

void RuleOfThree::VisitDecl(clang::Decl *decl)
{
    CXXRecordDecl *record = dyn_cast<CXXRecordDecl>(decl);
    if (!record || isBlacklisted(record) || !record->hasDefinition() || record->isPolymorphic())
        return;

    // fwd decl is not interesting
    if (record != record->getDefinition())
        return;

    if (shouldIgnoreFile(decl->getLocStart()))
        return;

    const SourceLocation recordStart = record->getLocStart();
    if (recordStart.isMacroID()) {
        if (clazy::isInMacro(&m_astContext, recordStart, "Q_GLOBAL_STATIC_INTERNAL"))
            return;
    }

    CXXConstructorDecl *copyCtor = Utils::copyCtor(record);
    CXXMethodDecl *copyAssign = Utils::copyAssign(record);
    CXXDestructorDecl *destructor = record->getDestructor();
    const bool dtorDefaultedByUser = destructor && destructor->isDefaulted() && !destructor->isImplicit();

    const bool hasUserCopyCtor = copyCtor && copyCtor->isUserProvided();
    const bool hasUserCopyAssign = copyAssign && copyAssign->isUserProvided();
    const bool hasUserDtor = destructor && destructor->isUserProvided();
    //const bool hasMoveCtor = record->hasNonTrivialMoveConstructor();
    //const bool hasTrivialMoveAssignment = record->hasNonTrivialMoveAssignment();

    const int numImplemented = hasUserCopyCtor + hasUserCopyAssign + hasUserDtor;
    if (numImplemented == 0 || numImplemented == 3) // Rule of 3 respected
        return;

    vector<StringRef> hasList;
    vector<StringRef> missingList;
    if (hasUserDtor)
        hasList.push_back("dtor");
    else
        missingList.push_back("dtor");

    if (hasUserCopyCtor)
        hasList.push_back("copy-ctor");
    else
        missingList.push_back("copy-ctor");

    if (hasUserCopyAssign)
        hasList.push_back("copy-assignment");
    else
        missingList.push_back("copy-assignment");

    const int numNotImplemented = missingList.size();

    const string className = record->getNameAsString();
    if (shouldIgnoreType(className))
        return;

    const string classQualifiedName = record->getQualifiedNameAsString();

    if (hasUserDtor && numImplemented == 1) {
        // Protected dtor is a way for a non-polymorphic base class avoid being deleted
        if (destructor->getAccess() == clang::AccessSpecifier::AS_protected)
            return;

        const bool copyCtorIsDeleted = copyCtor && copyCtor->isDeleted();
        const bool copyAssignIsDeleted = copyAssign && copyAssign->isDeleted();

        if (copyCtorIsDeleted && copyAssignIsDeleted) // They were explicitely deleted, it's safe.
            return;

        if (Utils::functionHasEmptyBody(destructor)) {
            // Lets reduce noise and allow the empty dtor. In theory we could warn, but there's no
            // hidden bug behind this dummy dtor.
            return;
        }
    }

    if (!hasUserDtor && (TypeUtils::derivesFrom(record, "QSharedData") || dtorDefaultedByUser))
        return;

    if (Utils::hasMember(record, "QSharedDataPointer"))
        return; // These need boiler-plate copy ctor and dtor

    const string filename = sm().getFilename(recordStart);
    if (clazy::endsWith(className, "Private") && clazy::endsWithAny(filename, { ".cpp", ".cxx", "_p.h" }))
        return; // Lots of RAII classes fall into this category. And even Private (d-pointer) classes, warning in that case would just be noise

    string msg = classQualifiedName + " has ";

    for (int i = 0; i < numImplemented; ++i) {
        msg += hasList[i];
        const bool isLast = i == numImplemented - 1;
        if (!isLast)
            msg += ',';
        msg += ' ';
    }

    msg += "but not ";
    for (int i = 0; i < numNotImplemented; ++i) {
        msg += missingList[i];
        const bool isLast = i == numNotImplemented - 1;
        if (!isLast)
            msg += ", ";
    }

    emitWarning(decl->getLocStart(), msg);
}

bool RuleOfThree::shouldIgnoreType(const std::string &className) const
{
    static const vector<StringRef> types = { "QTransform" // Fixed for Qt 6
                                        };

    return clazy::contains(types, className);
}
