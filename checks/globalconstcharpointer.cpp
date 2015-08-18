/**********************************************************************
**  Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
**  Author: Sérgio Martins <sergio.martins@kdab.com>
**
** This file may be distributed and/or modified under the terms of the
** GNU Lesser General Public License version 2.1 and version 3 as published by the
** Free Software Foundation and appearing in the file LICENSE.LGPL.txt included.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**********************************************************************/

#include "globalconstcharpointer.h"
#include "checkmanager.h"

#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>

using namespace clang;

GlobalConstCharPointer::GlobalConstCharPointer(const std::string &name)
    : CheckBase(name)
{
}

void GlobalConstCharPointer::VisitDecl(clang::Decl *decl)
{
    VarDecl *varDecl = dyn_cast<VarDecl>(decl);
    if (varDecl == nullptr || !varDecl->hasGlobalStorage() || varDecl->isCXXClassMember() || !varDecl->hasExternalFormalLinkage() || decl->isInAnonymousNamespace())
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

REGISTER_CHECK("global-const-char-pointer", GlobalConstCharPointer)
