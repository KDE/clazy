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
#include "checkmanager.h"
#include "MethodSignatureUtils.h"

#include <clang/AST/AST.h>

using namespace clang;
using namespace std;


CtorMissingParentArgument::CtorMissingParentArgument(const std::string &name, const clang::CompilerInstance &ci)
    : CheckBase(name, ci)
{
}

void CtorMissingParentArgument::VisitDecl(Decl *decl)
{
    auto record = dyn_cast<CXXRecordDecl>(decl);
    if (!record || !record->hasDefinition() ||
        record->getDefinition() != record || // Means fwd decl
        !QtUtils::isQObject(record)) {

        return;
    }

    bool foundParentArgument = false;
    for (auto ctor : record->ctors()) {
        if (ctor->isCopyOrMoveConstructor())
            continue;

        for (auto param : ctor->parameters()) {
            QualType qt = TypeUtils::pointeeQualType(param->getType());
            if (!qt.isConstQualified() && QtUtils::isQObject(qt)) {
                foundParentArgument = true;
                break;
            }
        }
    }

    if (!foundParentArgument) {
        emitWarning(decl, record->getQualifiedNameAsString() + string(" should take QObject parent argument in CTOR"));
    }
}


REGISTER_CHECK_WITH_FLAGS("ctor-missing-parent-argument", CtorMissingParentArgument, CheckLevel2)
