/*
    Copyright (C) 2026 Author <your@email>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "modernize-overloaded-signals.h"
#include "HierarchyUtils.h"
#include "QtUtils.h"
#include "TypeUtils.h"
#include "Utils.h"

#include <clang/AST/AST.h>

using namespace clang;

ModernizeOverloadedSignals::ModernizeOverloadedSignals(const std::string &name)
    : CheckBase(name)
{
}

void ModernizeOverloadedSignals::VisitDecl(clang::Decl *decl)
{
}

void ModernizeOverloadedSignals::VisitStmt(clang::Stmt *stmt)
{
}
