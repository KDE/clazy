/*
   This file is part of the clang-lazy static checker.

  Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
  Author: Sérgio Martins <sergio.martins@kdab.com>

  Copyright (C) 2015 Sergio Martins <smartins@kde.org>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

  As a special exception, permission is given to link this program
  with any edition of Qt, and distribute the resulting executable,
  without including the source code for Qt in the source distribution.
*/

#include "oldstyleconnect.h"
#include "Utils.h"
#include "checkmanager.h"

#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/AST.h>
#include <clang/AST/DeclTemplate.h>

using namespace clang;
using namespace std;


OldStyleConnect::OldStyleConnect(const std::string &name)
    : CheckBase(name)
{
}

static bool isOldStyleConnect(FunctionDecl *func)
{
    for (auto it = func->param_begin(), e = func->param_end(); it != e; ++it) {
        ParmVarDecl *parm = *it;
        QualType qt = parm->getType();
        const Type *t = qt.getTypePtrOrNull();
        if (!t || !t->isPointerType())
            continue;

        const Type *ptt = t->getPointeeType().getTypePtrOrNull();
        if (ptt && ptt->isCharType())
            return true;
    }

    return false;
}

void OldStyleConnect::VisitStmt(Stmt *s)
{
    CallExpr *call = dyn_cast<CallExpr>(s);
    if (!call)
        return;

    if (m_lastMethodDecl && m_lastMethodDecl->getParent() && m_lastMethodDecl->getParent()->getNameAsString() == "QObject") // Don't warn of stuff inside qobject.h
        return;

    FunctionDecl *function = call->getDirectCallee();
    if (!function)
        return;

    CXXMethodDecl *method = dyn_cast<CXXMethodDecl>(function);
    if (!method)
        return;

    const string methodName = method->getQualifiedNameAsString();

    if (methodName != "QObject::connect" && methodName != "QObject::disconnect" && methodName != "QTimer::singleShot") {
        return;
    }

    if (!isOldStyleConnect(method))
        return;

    emitWarning(s->getLocStart(), "Old Style Connect");
}

REGISTER_CHECK_WITH_FLAGS("old-style-connect", OldStyleConnect, NoFlag)
