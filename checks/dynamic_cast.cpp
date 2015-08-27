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

#include "dynamic_cast.h"
#include "Utils.h"
#include "checkmanager.h"

#include <clang/AST/DeclCXX.h>
#include <clang/AST/ExprCXX.h>

using namespace clang;

BogusDynamicCast::BogusDynamicCast(const std::string &name)
    : CheckBase(name)
{
}

void BogusDynamicCast::VisitStmt(clang::Stmt *stm)
{
    auto dynExp = dyn_cast<CXXDynamicCastExpr>(stm);
    if (dynExp == nullptr)
        return;

    auto namedCast = dyn_cast<CXXNamedCastExpr>(stm);
    CXXRecordDecl *castFrom = Utils::namedCastInnerDecl(namedCast);
    if (castFrom == nullptr)
        return;

    //if (Utils::isQObject(castFrom)) // Very noisy and not very useful, and qobject_cast can fail too
        //emitWarning(dynExp->getLocStart(), "Use qobject_cast rather than dynamic_cast");

    //if (dynExp->isAlwaysNull()) { // Crashing in Type.h  assert(isa<T>(CanonicalType))
      //  emitWarning(dynExp->getLocStart(), "That dynamic_cast is always null");
//        return;
  //  }

    CXXRecordDecl *castTo = Utils::namedCastOuterDecl(namedCast);
    if (castTo == nullptr)
        return;

    if (castFrom == castTo) {
        emitWarning(stm->getLocStart(), "Casting to itself");
    } else if (Utils::isChildOf(/*child=*/castFrom, castTo)) {
        emitWarning(stm->getLocStart(), "explicitly casting to base is unnecessary");
    }
}

REGISTER_CHECK("bogus-dynamic-cast", BogusDynamicCast)
