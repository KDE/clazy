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

#include "qmap-with-pointer-key.h"
#include "Utils.h"
#include "SourceCompatibilityHelpers.h"
#include "StringUtils.h"

#include <clang/AST/DeclBase.h>
#include <clang/AST/DeclTemplate.h>
#include <clang/AST/TemplateBase.h>
#include <clang/AST/Type.h>
#include <llvm/ADT/StringRef.h>

class ClazyContext;

using namespace clang;
using namespace std;

QMapWithPointerKey::QMapWithPointerKey(const std::string &name, ClazyContext *context)
    : CheckBase(name, context)
{
}

void QMapWithPointerKey::VisitDecl(clang::Decl *decl)
{
    auto tsdecl = Utils::templateSpecializationFromVarDecl(decl);
    if (!tsdecl || clazy::name(tsdecl) != "QMap")
        return;

    const TemplateArgumentList &templateArguments = tsdecl->getTemplateArgs();
    if (templateArguments.size() != 2)
        return;

    QualType qt = templateArguments[0].getAsType();
    const Type *t = qt.getTypePtrOrNull();
    if (t && t->isPointerType()) {
        emitWarning(clazy::getLocStart(decl), "Use QHash<K,T> instead of QMap<K,T> when K is a pointer");
    }
}
