/*
  This file is part of the clazy static checker.

  Copyright (C) 2021 Waqar Ahmed <waqar.17a@gmail.com>

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

#include "use-arrow-operator.h"

#include "HierarchyUtils.h"
#include <clang/AST/ExprCXX.h>

using namespace std;
using namespace clang;


UseArrowOperator::UseArrowOperator(const std::string &name, ClazyContext *context)
    : CheckBase(name, context)
{
}

void UseArrowOperator::VisitStmt(clang::Stmt *stmt)
{
    auto ce = dyn_cast<CXXMemberCallExpr>(stmt);
    if (!ce) {
        return;
    }

    auto vec = Utils::callListForChain(ce);
    if (vec.size() < 2) {
        return;
    }

    FunctionDecl* funcDecl = vec.at(vec.size() - 1)->getDirectCallee();
    if (!funcDecl) {
        return;
    }
    const std::string func = clazy::qualifiedMethodName(funcDecl);

    static const std::vector<std::string> whiteList {
        "QScopedPointer::data",
        "QPointer::data",
        "QSharedPointer::data",
        "QSharedDataPointer::data"
    };

    bool accepted = clazy::any_of(whiteList, [func](const std::string& f) { return f == func; });
    if (!accepted) {
        return;
    }

    emitWarning(clazy::getLocStart(stmt), "readibility: Use operator -> directly instead of data()->");
}
