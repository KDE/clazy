/*
    SPDX-FileCopyrightText: 2015 Klarälvdalens Datakonsult AB a KDAB Group company info@kdab.com
    SPDX-FileContributor: Sérgio Martins <sergio.martins@kdab.com>

    SPDX-FileCopyrightText: 2015 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLAZY_RESERVE_CANDIDATES
#define CLAZY_RESERVE_CANDIDATES

#include "checkbase.h"

#include <string>
#include <vector>

class ClazyContext;

namespace clang
{
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
class ReserveCandidates : public CheckBase
{
public:
    ReserveCandidates(const std::string &name, ClazyContext *context);
    void VisitStmt(clang::Stmt *stm) override;

private:
    bool registerReserveStatement(clang::Stmt *stmt);
    bool containerWasReserved(clang::ValueDecl *) const;
    bool acceptsValueDecl(clang::ValueDecl *valueDecl) const;
    bool expressionIsComplex(clang::Expr *) const;
    bool loopIsComplex(clang::Stmt *, bool &isLoop) const;
    bool isInComplexLoop(clang::Stmt *, clang::SourceLocation declLocation, bool isMemberVariable) const;
    bool isReserveCandidate(clang::ValueDecl *valueDecl, clang::Stmt *loopBody, clang::CallExpr *callExpr) const;

    std::vector<clang::ValueDecl *> m_foundReserves;
};

#endif
