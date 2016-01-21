/*
   This file is part of the clang-lazy static checker.

  Copyright (C) 2015 Sergio Martins <smartins@kde.org>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

  As a special exception, permission is given to link this program
  with any edition of Qt, and distribute the resulting executable,
  without including the source code for Qt in the source distribution.
*/

#include "ruleofthree.h"
#include "Utils.h"
#include "checkmanager.h"
#include "StringUtils.h"

#include <clang/AST/AST.h>

using namespace clang;
using namespace std;

RuleOfThree::RuleOfThree(const std::string &name, const clang::CompilerInstance &ci)
    : RuleOfBase(name, ci)
{
}

void RuleOfThree::VisitDecl(clang::Decl *decl)
{
    CXXRecordDecl *record = dyn_cast<CXXRecordDecl>(decl);
    if (!record || isBlacklisted(record) || !record->hasDefinition() || record->isPolymorphic())
        return;

    // fwd decl is not interesting
    if (record != record->getDefinition())
        return;

    const SourceLocation recordStart = record->getLocStart();
    if (recordStart.isMacroID()) {
        if (Utils::isInMacro(m_ci, recordStart, "Q_GLOBAL_STATIC_INTERNAL")) {
            return;
        }
    }

    CXXConstructorDecl *copyCtor = Utils::copyCtor(record);
    CXXMethodDecl *copyAssign = Utils::copyAssign(record);
    CXXDestructorDecl *destructor = record->getDestructor();
    const bool dtorDefaultedByUser = destructor && destructor->isDefaulted() && !destructor->isImplicit();

    const bool hasCopyCtor = copyCtor && copyCtor->isUserProvided();
    const bool hasCopyAssign = copyAssign && copyAssign->isUserProvided();
    const bool hasDtor = destructor && destructor->isUserProvided();
    //const bool hasMoveCtor = record->hasNonTrivialMoveConstructor();
    //const bool hasTrivialMoveAssignment = record->hasNonTrivialMoveAssignment();

    const int numImplemented = hasCopyCtor + hasCopyAssign + hasDtor;
    if (numImplemented == 0 || numImplemented == 3) // Rule of 3 respected
        return;

    vector<string> hasList;
    vector<string> missingList;
    if (hasDtor)
        hasList.push_back("dtor");
    else
        missingList.push_back("dtor");

    if (hasCopyCtor)
        hasList.push_back("copy-ctor");
    else
        missingList.push_back("copy-ctor");

    if (hasCopyAssign)
        hasList.push_back("copy-assignment");
    else
        missingList.push_back("copy-assignment");

    const int numNotImplemented = missingList.size();

    const string className = record->getNameAsString();
    const string classQualifiedName = record->getQualifiedNameAsString();

    if (hasDtor && numImplemented == 1) {
        // Protected dtor is a way for a non-polymorphic base class avoid being deleted
        if (destructor->getAccess() == clang::AccessSpecifier::AS_protected)
            return;

        const bool copyCtorIsDeleted = copyCtor && copyCtor->isDeleted();
        const bool copyAssignIsDeleted = copyAssign && copyAssign->isDeleted();

        if (copyCtorIsDeleted && copyAssignIsDeleted) // They were explicitely deleted, it's safe.
            return;
    }

    if (!hasDtor) {
        if (Utils::descendsFrom(record, "QSharedData"))
            return;

        if (dtorDefaultedByUser)
            return;
    }

    if (Utils::hasMember(record, "QSharedDataPointer"))
        return; // These need boiler-plate copy ctor and dtor

    const string filename = m_ci.getSourceManager().getFilename(recordStart);
    if (stringEndsWith(className, "Private") && (stringEndsWith(filename, ".cpp") || stringEndsWith(filename, ".cxx") || stringEndsWith(filename, "_p.h")))
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

std::vector<string> RuleOfThree::filesToIgnore() const
{
    static const std::vector<string> files = { "qrc_" };
    return files;
}

REGISTER_CHECK_WITH_FLAGS("rule-of-three", RuleOfThree, CheckLevel2)
