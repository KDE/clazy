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
