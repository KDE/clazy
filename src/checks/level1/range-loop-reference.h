/*
    SPDX-FileCopyrightText: 2015 Klarälvdalens Datakonsult AB a KDAB Group company info@kdab.com
    SPDX-FileContributor: Sérgio Martins <sergio.martins@kdab.com>

    SPDX-FileCopyrightText: 2015 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef RANGELOOP_REFERENCE_H
#define RANGELOOP_REFERENCE_H

#include "checkbase.h"

namespace clang
{
class CXXForRangeStmt;
}

/**
 * Finds places where you're using C++11 for range loops with Qt containers. (potential detach)
 */
class RangeLoopReference : public CheckBase
{
public:
    using CheckBase::CheckBase;
    void VisitStmt(clang::Stmt *stmt) override;

private:
    bool islvalue(clang::Expr *exp, clang::SourceLocation &endLoc);
    void processForRangeLoop(clang::CXXForRangeStmt *rangeLoop);
    void checkPassByConstRefCorrectness(clang::CXXForRangeStmt *rangeLoop);
};

#endif
