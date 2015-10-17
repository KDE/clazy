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

#include "implicitcasts.h"
#include "Utils.h"
#include "checkmanager.h"
#include "StringUtils.h"

#include <clang/AST/AST.h>
#include <clang/Lex/Lexer.h>

using namespace clang;
using namespace std;


ImplicitCasts::ImplicitCasts(const std::string &name)
    : CheckBase(name)
{

}

void ImplicitCasts::VisitStmt(clang::Stmt *stmt)
{
    auto implicitCast = dyn_cast<ImplicitCastExpr>(stmt);
    if (!implicitCast)
        return;

    if (checkFromBoolImplicitCast(implicitCast))
        return;

}

std::vector<string> ImplicitCasts::filesToIgnore() const
{
    static vector<string> files = {"/gcc/", "/c++/", "functional_hash.h", "qobject_impl.h", "qdebug.h",
                                   "hb-", "qdbusintegrator.cpp", "harfbuzz-", "qunicodetools.cpp"};
    return files;
}

bool ImplicitCasts::checkFromBoolImplicitCast(ImplicitCastExpr *implicitCast)
{
    if (implicitCast->getCastKind() == clang::CK_LValueToRValue)
        return false;

    if (implicitCast->getType().getTypePtrOrNull()->isBooleanType())
        return false;

    Expr *expr = implicitCast->getSubExpr();
    QualType qt = expr->getType();

    if (!qt.getTypePtrOrNull()->isBooleanType()) // Filter out some bool to const bool
        return false;

    Stmt *p = Utils::parent(m_parentMap, implicitCast);
    if (p && isa<BinaryOperator>(p))
        return false;

    if (p && (isa<CStyleCastExpr>(p) || isa<CXXFunctionalCastExpr>(p)))
        return false;

    if (Utils::isInsideOperatorCall(m_parentMap, implicitCast, {"QTextStream", "QAtomicInt", "QBasicAtomicInt"}))
        return false;

    if (Utils::insideCTORCall(m_parentMap, implicitCast, {"QAtomicInt", "QBasicAtomicInt"}))
        return false;

    if (!Utils::parent(m_parentMap, implicitCast))
        return false;


    EnumConstantDecl *enumerator = m_lastDecl ? dyn_cast<EnumConstantDecl>(m_lastDecl) : nullptr;
    if (enumerator) {
        // False positive in Qt headers which generates a lot of noise
        return false;
    }

    auto macro = Lexer::getImmediateMacroName(implicitCast->getLocStart(), m_ci.getSourceManager(), m_ci.getLangOpts());
    if (macro == "Q_UNLIKELY" || macro == "Q_LIKELY") {
        return false;
    }

    emitWarning(implicitCast->getLocStart(), "Implicit cast from bool");

    return true;
}

REGISTER_CHECK_WITH_FLAGS("implicit-casts", ImplicitCasts, HiddenFlag)
