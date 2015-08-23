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

#ifndef RESERVE_ADVISOR
#define RESERVE_ADVISOR

#include "checkbase.h"

#include <vector>

namespace clang {
class ValueDecl;
class Expr;
}

/**
 * Recommends places that are missing QList::reserve() or QVector::reserve().
 *
 * Only local variables are contemplated, containers that are members of a class are ignored due to
 * high false-positive rate.
 *
 * There some chance of false-positives.
 */
class ReserveAdvisor : public CheckBase
{
public:
    ReserveAdvisor(const std::string &name);
    void VisitStmt(clang::Stmt *stm) override;

private:
    void checkIfReserveStatement(clang::Stmt *stmt);
    bool containerWasReserved(clang::ValueDecl*) const;
    bool acceptsValueDecl(clang::ValueDecl *valueDecl) const;
    void printWarning(const clang::SourceLocation &);
    bool expressionIsTooComplex(clang::Expr *) const;
    bool loopIsTooComplex(clang::Stmt *) const;
    bool isInComplexLoop(clang::Stmt *, clang::SourceLocation declLocation) const;

    std::vector<clang::ValueDecl*> m_foundReserves;
};

#endif
