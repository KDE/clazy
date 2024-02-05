/*
    SPDX-FileCopyrightText: 2016 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "mutable-container-key.h"
#include "StringUtils.h"
#include "Utils.h"
#include "clazy_stl.h"

#include <clang/AST/DeclBase.h>
#include <clang/AST/DeclTemplate.h>
#include <clang/AST/TemplateBase.h>
#include <clang/AST/Type.h>
#include <llvm/ADT/StringRef.h>

#include <vector>

class ClazyContext;

using namespace clang;

static bool isInterestingContainer(StringRef name)
{
    static const std::vector<StringRef> containers = {"QMap", "QHash"};
    return clazy::contains(containers, name);
}

MutableContainerKey::MutableContainerKey(const std::string &name, ClazyContext *context)
    : CheckBase(name, context, Option_CanIgnoreIncludes)
{
}

void MutableContainerKey::VisitDecl(clang::Decl *decl)
{
    auto *tsdecl = Utils::templateSpecializationFromVarDecl(decl);
    if (!tsdecl || !isInterestingContainer(clazy::name(tsdecl))) {
        return;
    }

    const TemplateArgumentList &templateArguments = tsdecl->getTemplateArgs();
    if (templateArguments.size() != 2) {
        return;
    }

    QualType qt = templateArguments[0].getAsType();
    const Type *t = qt.getTypePtrOrNull();
    if (!t) {
        return;
    }

    auto *record = t->isRecordType() ? t->getAsCXXRecordDecl() : nullptr;
    if (!clazy::classIsOneOf(record, {"QPointer", "QWeakPointer", "QPersistentModelIndex", "weak_ptr"})) {
        return;
    }

    emitWarning(decl->getBeginLoc(), "Associative container key might be modified externally");
}
