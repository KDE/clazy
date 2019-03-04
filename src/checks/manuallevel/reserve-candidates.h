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

#ifndef CLAZY_RESERVE_CANDIDATES
#define CLAZY_RESERVE_CANDIDATES

#include "checkbase.h"

#include <vector>
#include <string>

class ClazyContext;

namespace clang {
class ValueDecl;
class Expr;
class CallExpr;
class SourceLocation;
class Stmt;
}

/**
 * Recommends places that are missing QList::reserve() or QVector::reserve().
 *
 * Only local variables are contemplated, containers that are members of a class are ignored due to
 * high false-positive rate.
 *
 * There some chance of false-positives.
 */
class ReserveCandidates
    : public CheckBase
{
public:
    ReserveCandidates(const std::string &name, ClazyContext *context);
    void VisitStmt(clang::Stmt *stm) override;

private:
    bool registerReserveStatement(clang::Stmt *stmt);
    bool containerWasReserved(clang::ValueDecl*) const;
    bool acceptsValueDecl(clang::ValueDecl *valueDecl) const;
    bool expressionIsComplex(clang::Expr *) const;
    bool loopIsComplex(clang::Stmt *, bool &isLoop) const;
    bool isInComplexLoop(clang::Stmt *, clang::SourceLocation declLocation, bool isMemberVariable) const;
    bool isReserveCandidate(clang::ValueDecl *valueDecl, clang::Stmt *loopBody, clang::CallExpr *callExpr) const;

    std::vector<clang::ValueDecl*> m_foundReserves;
};

#endif
