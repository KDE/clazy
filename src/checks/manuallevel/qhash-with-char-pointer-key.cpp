/*
  This file is part of the clazy static checker.

  Copyright (C) 2018 Sergio Martins <smartins@kde.org>

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

#include "qhash-with-char-pointer-key.h"
#include "Utils.h"
#include "HierarchyUtils.h"
#include "QtUtils.h"
#include "TypeUtils.h"

#include <clang/AST/AST.h>

using namespace clang;
using namespace std;


QHashWithCharPointerKey::QHashWithCharPointerKey(const std::string &name, ClazyContext *context)
    : CheckBase(name, context)
{
}

void QHashWithCharPointerKey::VisitDecl(clang::Decl *decl)
{
    auto tsdecl = Utils::templateSpecializationFromVarDecl(decl);
    if (!tsdecl || tsdecl->getName() != "QHash") // For QMap you shouldn't use any kind of pointers, that's handled in another check
        return;

    const TemplateArgumentList &templateArguments = tsdecl->getTemplateArgs();
    if (templateArguments.size() != 2)
        return;

    QualType qt = templateArguments[0].getAsType();
    if (!qt.isNull() && qt->isPointerType()) {
        qt = TypeUtils::pointeeQualType(qt);
        if (!qt.isNull() && !qt->isPointerType() && qt->isCharType()) {
            emitWarning(getLocStart(decl), "Using QHash<const char *, T> is dangerous");
        }
    }
}
