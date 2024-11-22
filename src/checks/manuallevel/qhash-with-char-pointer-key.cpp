/*
    SPDX-FileCopyrightText: 2018 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "qhash-with-char-pointer-key.h"
#include "StringUtils.h"
#include "TypeUtils.h"
#include "Utils.h"

#include <clang/AST/DeclBase.h>
#include <clang/AST/DeclTemplate.h>
#include <clang/AST/TemplateBase.h>
#include <clang/AST/Type.h>
#include <llvm/ADT/StringRef.h>

using namespace clang;

QHashWithCharPointerKey::QHashWithCharPointerKey(const std::string &name, ClazyContext *context)
    : CheckBase(name, context)
{
}

void QHashWithCharPointerKey::VisitDecl(clang::Decl *decl)
{
    auto *tsdecl = Utils::templateSpecializationFromVarDecl(decl);
    if (!tsdecl || clazy::name(tsdecl) != "QHash") { // For QMap you shouldn't use any kind of pointers, that's handled in another check
        return;
    }

    const TemplateArgumentList &templateArguments = tsdecl->getTemplateArgs();
    if (templateArguments.size() != 2) {
        return;
    }

    QualType qt = templateArguments[0].getAsType();
    if (!qt.isNull() && qt->isPointerType()) {
        qt = clazy::pointeeQualType(qt);
        if (!qt.isNull() && !qt->isPointerType() && qt->isCharType()) {
            emitWarning(decl->getBeginLoc(), "Using QHash<const char *, T> is dangerous");
        }
    }
}
