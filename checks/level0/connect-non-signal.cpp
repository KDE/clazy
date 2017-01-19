/*
  This file is part of the clazy static checker.

  Copyright (C) 2016 Sergio Martins <smartins@kde.org>
  Copyright (C) 2016 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.coms

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

#include "connect-non-signal.h"

#if !defined(IS_OLD_CLANG)

#include "AccessSpecifierManager.h"
#include "Utils.h"
#include "HierarchyUtils.h"
#include "QtUtils.h"
#include "TypeUtils.h"
#include "checkmanager.h"

#include <clang/AST/AST.h>

using namespace clang;
using namespace std;


ConnectNonSignal::ConnectNonSignal(const std::string &name, const clang::CompilerInstance &ci)
    : CheckBase(name, ci)
{
    CheckManager::instance()->enableAccessSpecifierManager(ci);
}

void ConnectNonSignal::VisitStmt(clang::Stmt *stmt)
{
    auto call = dyn_cast<CallExpr>(stmt);
    if (!call)
        return;

    FunctionDecl *func = call->getDirectCallee();
    if (!QtUtils::isConnect(func) || !QtUtils::connectHasPMFStyle(func))
        return;

    CXXMethodDecl *method = QtUtils::pmfFromConnect(call, /*argIndex=*/ 1);
    if (!method) {
        emitInternalError(func->getLocStart(), "couldn't find method from pmf connect");
        return;
    }

    QtAccessSpecifierType qst = CheckManager::instance()->accessSpecifierManager()->qtAccessSpecifierType(method);
    if (qst != QtAccessSpecifier_Unknown && qst != QtAccessSpecifier_Signal)
        emitWarning(call, method->getQualifiedNameAsString() + string(" is not a signal"));
}

REGISTER_CHECK_WITH_FLAGS("connect-non-signal", ConnectNonSignal, CheckLevel0)

#endif
