/*
    SPDX-FileCopyrightText: 2015 Klarälvdalens Datakonsult AB a KDAB Group company info@kdab.com
    SPDX-FileContributor: Sérgio Martins <sergio.martins@kdab.com>

    SPDX-FileCopyrightText: 2015 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "qmap-with-pointer-key.h"
#include "SourceCompatibilityHelpers.h"
#include "StringUtils.h"
#include "Utils.h"

#include <clang/AST/DeclBase.h>
#include <clang/AST/DeclTemplate.h>
#include <clang/AST/TemplateBase.h>
#include <clang/AST/Type.h>
#include <llvm/ADT/StringRef.h>

class ClazyContext;

using namespace clang;

QMapWithPointerKey::QMapWithPointerKey(const std::string &name, ClazyContext *context)
    : CheckBase(name, context)
{
}

void QMapWithPointerKey::VisitDecl(clang::Decl *decl)
{
    auto *tsdecl = Utils::templateSpecializationFromVarDecl(decl);
    if (!tsdecl || clazy::name(tsdecl) != "QMap") {
        return;
    }

    const TemplateArgumentList &templateArguments = tsdecl->getTemplateArgs();
    if (templateArguments.size() != 2) {
        return;
    }

    QualType qt = templateArguments[0].getAsType();
    const Type *t = qt.getTypePtrOrNull();
    if (t && t->isPointerType()) {
        emitWarning(clazy::getLocStart(decl), "Use QHash<K,T> instead of QMap<K,T> when K is a pointer");
    }
}
