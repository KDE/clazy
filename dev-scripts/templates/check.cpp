/*
    %3

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "%1"
#include "Utils.h"
#include "HierarchyUtils.h"
#include "QtUtils.h"
#include "TypeUtils.h"

#include <clang/AST/AST.h>

using namespace clang;

%2::%2(const std::string &name)
    : CheckBase(name)
{
}

void %2::VisitDecl(clang::Decl *decl)
{
}

void %2::VisitStmt(clang::Stmt *stmt)
{
}
