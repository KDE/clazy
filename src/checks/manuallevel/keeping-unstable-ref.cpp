/*
  This file is part of the clazy static checker.

  Copyright (C) 2020 Sergio Martins <smartins@kde.org>

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

#include "keeping-unstable-ref.h"
#include "Utils.h"
#include "HierarchyUtils.h"
#include "QtUtils.h"
#include "TypeUtils.h"
#include "ClazyContext.h"

#include <clang/AST/AST.h>

using namespace clang;
using namespace std;


KeepingUnstableRef::KeepingUnstableRef(const std::string &name, ClazyContext *context)
    : CheckBase(name, context)
{
}

void KeepingUnstableRef::VisitStmt(clang::Stmt *stmt)
{
    auto subscriptOp = dyn_cast<CXXOperatorCallExpr>(stmt); // operator[]
    if (!subscriptOp || subscriptOp->getOperator() != OO_Subscript)
        return;

    auto subscriptMethod = dyn_cast<CXXMethodDecl>(subscriptOp->getCalleeDecl());
    if (!subscriptMethod)
        return;

    CXXRecordDecl *classDecl = subscriptMethod->getParent();
    if (!classDecl)
        return;

    StringRef className = classDecl->getName();
    if (className != "QMap" && className != "QList")
        return;

    //if (processDeclRefCase(subscriptOp))
      //  return;

    if (processMemberAssignment(subscriptOp))
        return;
}

bool KeepingUnstableRef::processDeclRefCase(CXXOperatorCallExpr *op)
{
    Stmt *s = op;
    DeclStmt *declStmt = nullptr;
    while (s) {
        s = clazy::parent(m_context->parentMap, s);
        if (!s)
            break;
        if ((declStmt = dyn_cast<DeclStmt>(s)))
            break;
    }

    if (!declStmt || !declStmt->isSingleDecl())
        return false;

    auto varDecl = dyn_cast<VarDecl>(declStmt->getSingleDecl());
    const QualType qtype = varDecl->getType();
    if (!varDecl || (!qtype->isReferenceType() && !qtype->isPointerType())) {
        return false;
    }

    emitWarning(op, "storing a reference to an unstable container element");
    return true;
}

bool KeepingUnstableRef::processMemberAssignment(CXXOperatorCallExpr *op)
{
    Stmt *s = op;
    BinaryOperator *binaryOp = nullptr;
    while (s) {
        s = clazy::parent(m_context->parentMap, s);
        if (!s)
            break;
        if ((binaryOp = dyn_cast<BinaryOperator>(s)))
            break;
    }

    if (!binaryOp || binaryOp->getOpcode() != BO_Assign)
        return false;

    const QualType qtype = binaryOp->getType();
    if (!qtype->isPointerType()) {
        return false;
    }

    emitWarning(op, "storing a reference to an unstable container element");
    return true;
}
