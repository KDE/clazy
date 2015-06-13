/**********************************************************************
**  Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
**  Author: Sérgio Martins <sergio.martins@kdab.com>
**
** This file may be distributed and/or modified under the terms of the
** GNU Lesser General Public License version 2.1 and version 3 as published by the
** Free Software Foundation and appearing in the file LICENSE.LGPL.txt included.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**********************************************************************/

#include "dynamic_cast.h"
#include "Utils.h"

#include <clang/AST/DeclCXX.h>
#include <clang/AST/ExprCXX.h>

using namespace clang;

BogusDynamicCast::BogusDynamicCast(clang::CompilerInstance &ci)
    : CheckBase(ci)
{
}

void BogusDynamicCast::VisitStmt(clang::Stmt *stm)
{
    auto dynExp = dyn_cast<CXXDynamicCastExpr>(stm);
    if (dynExp == nullptr)
        return;

    SourceManager &sm = m_ci.getSourceManager();
    if (shouldIgnoreFile(sm.getFilename(stm->getLocStart())))
        return;

    auto namedCast = dyn_cast<CXXNamedCastExpr>(stm);
    CXXRecordDecl *castFrom = Utils::namedCastInnerDecl(namedCast);
    if (castFrom == nullptr)
        return;

    if (Utils::isQObject(castFrom))
        emitWarning(dynExp->getLocStart(), "Use qobject_cast rather than dynamic_cast [-Wmore-warnings-bogus-dynamic_cast]");

    //if (dynExp->isAlwaysNull()) { // Crashing in Type.h  assert(isa<T>(CanonicalType))
      //  emitWarning(dynExp->getLocStart(), "That dynamic_cast is always null [-Wmore-warnings-bogus-dynamic_cast]");
//        return;
  //  }

    CXXRecordDecl *castTo = Utils::namedCastOuterDecl(namedCast);
    if (castTo == nullptr)
        return;

    if (castFrom == castTo) {
        emitWarning(stm->getLocStart(), "Casting to itself [-Wmore-warnings-bogus-dynamic_cast]");
    } else if (Utils::isChildOf(/*child=*/castFrom, castTo)) {
        emitWarning(stm->getLocStart(), "explicitly casting to base is unnecessary [-Wmore-warnings-bogus-dynamic_cast]");
    }
}

std::string BogusDynamicCast::name() const
{
    return "bogus-dynamic-cast";
}
