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
#include "HierarchyUtils.h"
#include "ClazyContext.h"
#include "SourceCompatibilityHelpers.h"

#include <clang/AST/DeclCXX.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/Expr.h>
#include <clang/AST/OperationKinds.h>
#include <clang/AST/Stmt.h>
#include <clang/Basic/SourceLocation.h>
#include <llvm/Support/Casting.h>

#include <iterator>

using namespace llvm;
using namespace clang;

UnneededCast::UnneededCast(const std::string &name, ClazyContext *context)
    : CheckBase(name, context, Option_CanIgnoreIncludes)
{
}

void UnneededCast::VisitStmt(clang::Stmt *stm)
{
    if (handleNamedCast(dyn_cast<CXXNamedCastExpr>(stm)))
        return;

    handleQObjectCast(stm);
}

bool UnneededCast::handleNamedCast(CXXNamedCastExpr *namedCast)
{
    if (!namedCast)
        return false;

    const bool isDynamicCast = isa<CXXDynamicCastExpr>(namedCast);
    const bool isStaticCast = isDynamicCast ? false : isa<CXXStaticCastExpr>(namedCast);

    if (!isDynamicCast && !isStaticCast)
        return false;

    if (clazy::getLocStart(namedCast).isMacroID())
        return false;

    CXXRecordDecl *castFrom = namedCast ? Utils::namedCastInnerDecl(namedCast) : nullptr;
    if (!castFrom || !castFrom->hasDefinition() || std::distance(castFrom->bases_begin(), castFrom->bases_end()) > 1)
        return false;

    if (isStaticCast) {
        if (auto implicitCast = dyn_cast<ImplicitCastExpr>(namedCast->getSubExpr())) {
            if (implicitCast->getCastKind() == CK_NullToPointer) {
                // static_cast<Foo*>(0) is OK, and sometimes needed
                return false;
            }
        }

        // static_cast to base is needed in ternary operators
        if (clazy::getFirstParentOfType<ConditionalOperator>(m_context->parentMap, namedCast) != nullptr)
            return false;
    }

    if (isDynamicCast && !isOptionSet("prefer-dynamic-cast-over-qobject") && clazy::isQObject(castFrom))
        emitWarning(clazy::getLocStart(namedCast), "Use qobject_cast rather than dynamic_cast");

    CXXRecordDecl *castTo = Utils::namedCastOuterDecl(namedCast);
    if (!castTo)
        return false;

    return maybeWarn(namedCast, castFrom, castTo);
}

bool UnneededCast::handleQObjectCast(Stmt *stm)
{
    CXXRecordDecl *castTo = nullptr;
    CXXRecordDecl *castFrom = nullptr;

    if (!clazy::is_qobject_cast(stm, &castTo, &castFrom))
        return false;

    return maybeWarn(stm, castFrom, castTo, /*isQObjectCast=*/ true);
}

bool UnneededCast::maybeWarn(Stmt *stmt, CXXRecordDecl *castFrom, CXXRecordDecl *castTo, bool isQObjectCast)
{
    castFrom = castFrom->getCanonicalDecl();
    castTo = castTo->getCanonicalDecl();

    if (castFrom == castTo) {
        emitWarning(clazy::getLocStart(stmt), "Casting to itself");
        return true;
    } else if (clazy::derivesFrom(/*child=*/ castFrom, castTo)) {
        if (isQObjectCast) {
            const bool isTernaryOperator = clazy::getFirstParentOfType<ConditionalOperator>(m_context->parentMap, stmt) != nullptr;
            if (isTernaryOperator) {
                emitWarning(clazy::getLocStart(stmt), "use static_cast instead of qobject_cast");
            } else {
                emitWarning(clazy::getLocStart(stmt), "explicitly casting to base is unnecessary");
            }
        } else {
            emitWarning(clazy::getLocStart(stmt), "explicitly casting to base is unnecessary");
        }

        return true;
    }

    return false;
}
