/*
    SPDX-FileCopyrightText: 2015 Klarälvdalens Datakonsult AB a KDAB Group company info@kdab.com
    SPDX-FileContributor: Sérgio Martins <sergio.martins@kdab.com>

    SPDX-FileCopyrightText: 2015 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef RANGELOOP_DETACHMENTS_H
#define RANGELOOP_DETACHMENTS_H

#include "checkbase.h"

#include <string>

namespace clang
{
class CXXForRangeStmt;
}

/**
 * Finds places where you're using C++11 for range loops with Qt containers. (potential detach)
 */
class RangeLoopDetach : public CheckBase
{
public:
    RangeLoopDetach(const std::string &name, ClazyContext *context);
    void VisitStmt(clang::Stmt *stmt) override;

private:
    bool islvalue(clang::Expr *exp, clang::SourceLocation &endLoc);
    void processForRangeLoop(clang::CXXForRangeStmt *rangeLoop);
};

#endif
