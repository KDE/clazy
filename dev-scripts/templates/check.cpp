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
using namespace std;


%2::%2(const std::string &name, ClazyContext *context)
    : CheckBase(name, context)
{
}

void %2::VisitDecl(clang::Decl *decl)
{
}

void %2::VisitStmt(clang::Stmt *stmt)
{
}
