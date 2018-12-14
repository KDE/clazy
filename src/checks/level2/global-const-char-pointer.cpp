/*
    This file is part of the clazy static checker.

    Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
    Author: Sérgio Martins <sergio.martins@kdab.com>

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

#include "global-const-char-pointer.h"
#include "SourceCompatibilityHelpers.h"

#include <clang/AST/Decl.h>
#include <clang/AST/DeclBase.h>
#include <clang/AST/Type.h>
#include <clang/Basic/LLVM.h>
#include <llvm/Support/Casting.h>

#include <vector>

class ClazyContext;

using namespace clang;

GlobalConstCharPointer::GlobalConstCharPointer(const std::string &name, ClazyContext *context)
    : CheckBase(name, context)
{
    m_filesToIgnore = { "3rdparty", "mysql.h", "qpicture.cpp" };
}

void GlobalConstCharPointer::VisitDecl(clang::Decl *decl)
{
    VarDecl *varDecl = dyn_cast<VarDecl>(decl);
    if (!varDecl || !varDecl->hasGlobalStorage() || varDecl->isCXXClassMember() ||
        !varDecl->hasExternalFormalLinkage() || decl->isInAnonymousNamespace() || varDecl->hasExternalStorage())
        return;

    if (shouldIgnoreFile(clazy::getLocStart(decl)))
        return;

    QualType qt = varDecl->getType();
    const Type *type = qt.getTypePtrOrNull();
    if (!type || !type->isPointerType() || qt.isConstQualified() || varDecl->isStaticLocal())
        return;

    QualType pointeeQt = type->getPointeeType();
    const Type *pointeeType = pointeeQt.getTypePtrOrNull();
    if (!pointeeType || !pointeeType->isCharType())
        return;

    emitWarning(clazy::getLocStart(decl), "non const global char *");
}
