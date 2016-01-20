/*
   This file is part of the clang-lazy static checker.

  Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
  Author: Sérgio Martins <sergio.martins@kdab.com>

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

#include "globalconstcharpointer.h"
#include "checkmanager.h"

#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>

using namespace clang;

GlobalConstCharPointer::GlobalConstCharPointer(const std::string &name, const clang::CompilerInstance &ci)
    : CheckBase(name, ci)
{
}

void GlobalConstCharPointer::VisitDecl(clang::Decl *decl)
{
    VarDecl *varDecl = dyn_cast<VarDecl>(decl);
    if (varDecl == nullptr || !varDecl->hasGlobalStorage() || varDecl->isCXXClassMember() ||
        !varDecl->hasExternalFormalLinkage() || decl->isInAnonymousNamespace() || varDecl->hasExternalStorage())
        return;

    QualType qt = varDecl->getType();
    const Type *type = qt.getTypePtrOrNull();
    if (type == nullptr || !type->isPointerType() || qt.isConstQualified() || varDecl->isStaticLocal())
        return;

    QualType pointeeQt = type->getPointeeType();
    const Type *pointeeType = pointeeQt.getTypePtrOrNull();
    if (pointeeType == nullptr || !pointeeType->isCharType())
        return;

    emitWarning(decl->getLocStart(), "non const global char *");
}

std::vector<std::string> GlobalConstCharPointer::filesToIgnore() const
{
    static const std::vector<std::string> files = {"errno.h", "getopt.h", "StdHeader.h", "3rdparty", "mysql.h"};
    return files;
}

REGISTER_CHECK_WITH_FLAGS("global-const-char-pointer", GlobalConstCharPointer, CheckLevel2)
