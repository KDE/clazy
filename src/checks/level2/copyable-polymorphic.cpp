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

#include "copyable-polymorphic.h"
#include "Utils.h"
#include "SourceCompatibilityHelpers.h"
#include "AccessSpecifierManager.h"
#include "ClazyContext.h"
#include "FixItUtils.h"
#include "StringUtils.h"

#include <clang/AST/DeclCXX.h>
#include <clang/Basic/LLVM.h>
#include <clang/Basic/Specifiers.h>
#include <llvm/Support/Casting.h>

class ClazyContext;
namespace clang {
class Decl;
}  // namespace clang

using namespace clang;
using namespace std;


/// Returns whether the class has non-private copy-ctor or copy-assign
static bool hasNonPrivateCopy(const CXXRecordDecl *record)
{
    CXXConstructorDecl *copyCtor = Utils::copyCtor(record);
    const bool hasCallableCopyCtor = copyCtor && !copyCtor->isDeleted() && copyCtor->getAccess() != clang::AS_private;
    if (!hasCallableCopyCtor) {
        CXXMethodDecl *copyAssign = Utils::copyAssign(record);
        const bool hasCallableCopyAssign = copyAssign && !copyAssign->isDeleted() && copyAssign->getAccess() != clang::AS_private;
        if (!hasCallableCopyAssign)
            return false;
    }

    return true;
}

CopyablePolymorphic::CopyablePolymorphic(const std::string &name, ClazyContext *context)
    : CheckBase(name, context)
{
    context->enableAccessSpecifierManager();
}

void CopyablePolymorphic::VisitDecl(clang::Decl *decl)
{
    auto record = dyn_cast<CXXRecordDecl>(decl);
    if (!record || !record->hasDefinition() || record->getDefinition() != record || !record->isPolymorphic())
        return;

    if (!hasNonPrivateCopy(record))
        return;

    emitWarning(clazy::getLocStart(record), "Polymorphic class " + record->getQualifiedNameAsString() + " is copyable. Potential slicing.", fixits(record));
}

vector<clang::FixItHint> CopyablePolymorphic::fixits(clang::CXXRecordDecl *record)
{
    vector<FixItHint> result;

#if LLVM_VERSION_MAJOR >= 11 // older llvm has problems with \n in the yaml file
    const StringRef className = clazy::name(record);

    // Insert Q_DISABLE_COPY(classname) in the private section if one exists,
    // otherwise at the end of the class declaration
    SourceLocation pos =
        m_context->accessSpecifierManager->firstLocationOfSection(
            clang::AccessSpecifier::AS_private, record);

    if (pos.isValid()) {
        pos = Lexer::findLocationAfterToken(pos, clang::tok::colon, sm(), lo(),
                                            false);
        result.push_back(clazy::createInsertion(
            pos, string("\n\tQ_DISABLE_COPY(") + className.data() + string(")")));
    } else {
        pos = record->getBraceRange().getEnd();
        result.push_back(clazy::createInsertion(
            pos, string("\tQ_DISABLE_COPY(") + className.data() + string(")\n")));
    }

    // If the class has a default constructor, then we need to readd it,
    // as the disabled copy constructor removes it.
    // Add it in the public section if one exists, otherwise add a
    // public section at the top of the class declaration.
    if (record->hasDefaultConstructor()) {
        pos = m_context->accessSpecifierManager->firstLocationOfSection(
            clang::AccessSpecifier::AS_public, record);
        if (pos.isInvalid()) {
            pos = record->getBraceRange().getBegin().getLocWithOffset(1);
            result.push_back(clazy::createInsertion(
                pos, string("\npublic:\n\t") + className.data() + string("() = default;")));
        }
        else {
            pos = Lexer::findLocationAfterToken(pos, clang::tok::colon, sm(), lo(),
                                                false);
            result.push_back(clazy::createInsertion(
                pos, string("\n\t") + className.data() + string("() = default;")));
        }
    }
#endif

    return result;
}
