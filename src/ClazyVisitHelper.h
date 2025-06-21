/*
    SPDX-FileCopyrightText: 2025 Alexander Lohnau <alexander.lohnau@gmx.de>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "checkmanager.h"

namespace clazy::VisitHelper
{
struct Visitors {
    CheckBase::List visitStmts;
    CheckBase::List visitDecls;
    CheckBase::List visitAllTypedefDecls;

    void addCheck(RegisteredCheck::Options options, CheckBase *check)
    {
        if (options & RegisteredCheck::Option_VisitsStmts) {
            visitStmts.emplace_back(check);
        }
        if (options & RegisteredCheck::Option_VisitsDecls) {
            visitDecls.emplace_back(check);
        }
        if (options & RegisteredCheck::Option_VisitAllTypeDefs) {
            visitAllTypedefDecls.push_back(check);
        }
    }
};

bool VisitDecl(clang::Decl *decl, ClazyContext *context, const Visitors &visitors);
bool VisitStmt(clang::Stmt *stmt, ClazyContext *context, const Visitors &visitors);
}
