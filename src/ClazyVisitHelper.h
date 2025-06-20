/*
    SPDX-FileCopyrightText: 2025 Alexander Lohnau <alexander.lohnau@gmx.de>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

namespace clazy::VisitHelper
{
bool VisitDecl(clang::Decl *decl,
               ClazyContext *context,
               const std::vector<CheckBase *> &checksToVisit,
               const std::vector<CheckBase *> &checksToVisitAllTypedefs);
bool VisitStmt(clang::Stmt *stmt, ClazyContext *context, const std::vector<CheckBase *> &checksToVisit);
}
