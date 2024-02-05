/*
    SPDX-FileCopyrightText: 2015 Klarälvdalens Datakonsult AB a KDAB Group company info@kdab.com
    SPDX-FileContributor: Sérgio Martins <sergio.martins@kdab.com>

    SPDX-FileCopyrightText: 2015 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
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
    m_filesToIgnore = {"3rdparty", "mysql.h", "qpicture.cpp"};
}

void GlobalConstCharPointer::VisitDecl(clang::Decl *decl)
{
    auto *varDecl = dyn_cast<VarDecl>(decl);
    if (!varDecl || !varDecl->hasGlobalStorage() || varDecl->isCXXClassMember() || !varDecl->hasExternalFormalLinkage() || decl->isInAnonymousNamespace()
        || varDecl->hasExternalStorage()) {
        return;
    }

    if (shouldIgnoreFile(decl->getBeginLoc())) {
        return;
    }

    QualType qt = varDecl->getType();
    const Type *type = qt.getTypePtrOrNull();
    if (!type || !type->isPointerType() || qt.isConstQualified() || varDecl->isStaticLocal()) {
        return;
    }

    QualType pointeeQt = type->getPointeeType();
    const Type *pointeeType = pointeeQt.getTypePtrOrNull();
    if (!pointeeType || !pointeeType->isCharType()) {
        return;
    }

    emitWarning(decl->getBeginLoc(), "non const global char *");
}
