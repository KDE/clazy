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

#include "mutable-container-key.h"
#include "Utils.h"
#include "StringUtils.h"
#include "SourceCompatibilityHelpers.h"
#include "clazy_stl.h"

#include <clang/AST/DeclBase.h>
#include <clang/AST/DeclTemplate.h>
#include <clang/AST/TemplateBase.h>
#include <clang/AST/Type.h>
#include <llvm/ADT/StringRef.h>

#include <vector>

class ClazyContext;

using namespace clang;
using namespace std;

static bool isInterestingContainer(StringRef name)
{
    static const vector<StringRef> containers = { "QMap", "QHash" };
    return clazy::contains(containers, name);
}

MutableContainerKey::MutableContainerKey(const std::string &name, ClazyContext *context)
    : CheckBase(name, context, Option_CanIgnoreIncludes)
{
}

void MutableContainerKey::VisitDecl(clang::Decl *decl)
{
    auto tsdecl = Utils::templateSpecializationFromVarDecl(decl);
    if (!tsdecl || !isInterestingContainer(clazy::name(tsdecl)))
        return;

    const TemplateArgumentList &templateArguments = tsdecl->getTemplateArgs();
    if (templateArguments.size() != 2)
        return;

    QualType qt = templateArguments[0].getAsType();
    const Type *t = qt.getTypePtrOrNull();
    if (!t)
        return;

    auto record = t->isRecordType() ? t->getAsCXXRecordDecl() : nullptr;
    if (!clazy::classIsOneOf(record, {"QPointer", "QWeakPointer",
                                      "QPersistentModelIndex", "weak_ptr"}))
        return;


    emitWarning(clazy::getLocStart(decl), "Associative container key might be modified externally");
}
