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

#include "assertwithsideeffects.h"
#include "Utils.h"
#include "checkmanager.h"

#include <clang/AST/Expr.h>

using namespace clang;

AssertWithSideEffects::AssertWithSideEffects(const std::string &name)
    : CheckBase(name)
{
}

void AssertWithSideEffects::VisitStmt(Stmt *stm)
{
    ConditionalOperator *co = dyn_cast<ConditionalOperator>(stm);
    if (co == nullptr)
        return;

    Expr *trueExpr = co->getTrueExpr();
    Expr *falseExpr = co->getFalseExpr();

    if (!trueExpr || !falseExpr) return;

    auto trueCall = dyn_cast<CallExpr>(trueExpr);
    auto falseCall = dyn_cast<CallExpr>(falseExpr);
    if (!trueCall || !falseCall) return;

    FunctionDecl *f1 = trueCall->getDirectCallee();
    FunctionDecl *f2 = falseCall->getDirectCallee();

    if (!f1 || !f2) return;

    //llvm::errs() << f1 << " " << f2 << "\n";

    if (!f1->getDeclName().isIdentifier() || !f2->getDeclName().isIdentifier()) // Otherwise f1->getName() asserts
        return;

    if (!((f1->getName() == "qt_assert" || f1->getName() == "qt_assert_x") && f2->getName() == "qt_noop"))
        return;

    // Expr::HasSideEffects() is very aggressive and has a lot of false positives, so, if it returns
    // false, believe it.
    if (!co->getCond()->HasSideEffects(m_ci.getASTContext()))
        return;

    // Now, look for UnaryOperators, BinaryOperators and function calls
    if (Utils::childsHaveSideEffects(co->getCond())) {
        emitWarning(co->getLocStart(), "Code inside Q_ASSERT has side-effects but won't be built in release mode");
    }
}


// REGISTER_CHECK("assert-with-side-effects", AssertWithSideEffects)
