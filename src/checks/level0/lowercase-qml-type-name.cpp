/*
  This file is part of the clazy static checker.

    Copyright (C) 2018 Sergio Martins <smartins@kde.org>

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

#include "lowercase-qml-type-name.h"
#include "Utils.h"
#include "HierarchyUtils.h"
#include "QtUtils.h"
#include "TypeUtils.h"

#include <clang/AST/AST.h>
#include <cctype>

using namespace clang;
using namespace std;


LowercaseQMlTypeName::LowercaseQMlTypeName(const std::string &name, ClazyContext *context)
    : CheckBase(name, context)
{
}

void LowercaseQMlTypeName::VisitStmt(clang::Stmt *stmt)
{
    auto callExpr = dyn_cast<CallExpr>(stmt);
    if (!callExpr)
        return;

    FunctionDecl *func = callExpr->getDirectCallee();
    if (!func)
        return;

    StringRef name = clazy::name(func);

    Expr *arg = nullptr;

    if (name == "qmlRegisterType" || name == "qmlRegisterUncreatableType")
        arg = callExpr->getNumArgs() <= 3 ? nullptr : callExpr->getArg(3);

    if (!arg)
        return;

    auto literal = clazy::getFirstChildOfType2<StringLiteral>(arg);
    if (!literal)
        return;

    StringRef str = literal->getString();

    if (str.empty() || !isupper(str[0])) {
        emitWarning(arg, "QML types must begin with uppercase");
    }
}
