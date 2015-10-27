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

#include "qmapkey.h"
#include "Utils.h"
#include "checkmanager.h"

#include <clang/AST/DeclCXX.h>
#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>

using namespace clang;
using namespace std;

QMapKeyChecker::QMapKeyChecker(const std::string &name)
    : CheckBase(name)
{
}

void QMapKeyChecker::VisitDecl(clang::Decl *decl)
{
    auto tsdecl = Utils::templateSpecializationFromVarDecl(decl);
    if (tsdecl == nullptr || tsdecl->getName() != "QMap")
        return;

    const TemplateArgumentList &templateArguments = tsdecl->getTemplateArgs();
    if (templateArguments.size() != 2)
        return;

    QualType qt = templateArguments[0].getAsType();
    const Type *t = qt.getTypePtrOrNull();
    if (t && t->isPointerType()) {
        emitWarning(decl->getLocStart(), "Use QHash<K,T> instead of QMap<K,T> when K is a pointer");
    }
}

REGISTER_CHECK_WITH_FLAGS("qmap-with-key-pointer", QMapKeyChecker, CheckLevel0)
