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


CopyablePolymorphic::CopyablePolymorphic(const std::string &name, ClazyContext *context)
    : CheckBase(name, context)
{
}

void CopyablePolymorphic::VisitDecl(clang::Decl *decl)
{
    auto record = dyn_cast<CXXRecordDecl>(decl);
    if (!record || !record->hasDefinition() || record->getDefinition() != record || !record->isPolymorphic())
        return;

    CXXConstructorDecl *copyCtor = Utils::copyCtor(record);
    const bool hasCallableCopyCtor = copyCtor && !copyCtor->isDeleted() && copyCtor->getAccess() != clang::AS_private;
    if (!hasCallableCopyCtor) {
        CXXMethodDecl *copyAssign = Utils::copyAssign(record);
        const bool hasCallableCopyAssign = copyAssign && !copyAssign->isDeleted() && copyAssign->getAccess() != clang::AS_private;
        if (!hasCallableCopyAssign)
            return;
    }

    emitWarning(clazy::getLocStart(record), "Polymorphic class " + record->getQualifiedNameAsString() + " is copyable. Potential slicing.");
}
