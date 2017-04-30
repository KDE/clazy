/*
  This file is part of the clazy static checker.

  Copyright (C) 2017 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
  Author: Sérgio Martins <sergio.martins@kdab.com>

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

#include "qlatin1string-non-ascii.h"
#include "Utils.h"
#include "HierarchyUtils.h"
#include "QtUtils.h"
#include "TypeUtils.h"
#include "checkmanager.h"

#include <clang/AST/AST.h>

using namespace clang;
using namespace std;


QLatin1StringNonAscii::QLatin1StringNonAscii(const std::string &name, ClazyContext *context)
    : CheckBase(name, context)
{
}

void QLatin1StringNonAscii::VisitStmt(clang::Stmt *stmt)
{
    auto constructExpr = dyn_cast<CXXConstructExpr>(stmt);
    CXXConstructorDecl *ctor = constructExpr ? constructExpr->getConstructor() : nullptr;

    if (!ctor || ctor->getQualifiedNameAsString() != "QLatin1String::QLatin1String")
        return;

    StringLiteral *lt = HierarchyUtils::getFirstChildOfType2<StringLiteral>(stmt);
    if (lt && !Utils::isAscii(lt))
        emitWarning(stmt, "QStringLiteral with non-ascii literal");
}

REGISTER_CHECK("qlatin1string-non-ascii", QLatin1StringNonAscii, CheckLevel1)
