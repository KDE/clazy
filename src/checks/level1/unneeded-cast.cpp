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

#include "unneeded-cast.h"
#include "Utils.h"
#include "QtUtils.h"
#include "TypeUtils.h"

#include <clang/AST/DeclCXX.h>
#include <clang/AST/ExprCXX.h>

using namespace clang;

UnneededCast::UnneededCast(const std::string &name, ClazyContext *context)
    : CheckBase(name, context, Option_CanIgnoreIncludes)
{
}

void UnneededCast::VisitStmt(clang::Stmt *stm)
{
    if (handleNamedCast(dyn_cast<CXXNamedCastExpr>(stm)))
        return;
}

bool UnneededCast::handleNamedCast(CXXNamedCastExpr *namedCast)
{
    CXXRecordDecl *castFrom = namedCast ? Utils::namedCastInnerDecl(namedCast) : nullptr;
    if (!castFrom)
        return false;

    const bool isDynamicCast = isa<CXXDynamicCastExpr>(namedCast);
    if (isDynamicCast && !isOptionSet("prefer-dynamic-cast-over-qobject") && clazy::isQObject(castFrom))
        emitWarning(namedCast->getLocStart(), "Use qobject_cast rather than dynamic_cast");

    CXXRecordDecl *castTo = Utils::namedCastOuterDecl(namedCast);
    if (!castTo)
        return false;

    if (castFrom == castTo) {
        emitWarning(namedCast->getLocStart(), "Casting to itself");
    } else if (TypeUtils::derivesFrom(/*child=*/castFrom, castTo)) {
        emitWarning(namedCast->getLocStart(), "explicitly casting to base is unnecessary");
    }

    return true;
}
