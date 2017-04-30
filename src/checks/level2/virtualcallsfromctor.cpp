/*
   This file is part of the clazy static checker.

  Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
  Author: Sérgio Martins <sergio.martins@kdab.com>

  Copyright (C) 2015 Sergio Martins <smartins@kde.org>

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

#include "virtualcallsfromctor.h"
#include "Utils.h"
#include "HierarchyUtils.h"
#include "checkmanager.h"

#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>

using namespace std;
using namespace clang;

VirtualCallsFromCTOR::VirtualCallsFromCTOR(const std::string &name, ClazyContext *context)
    : CheckBase(name, context)
{

}

void VirtualCallsFromCTOR::VisitStmt(clang::Stmt *stm)
{

}

void VirtualCallsFromCTOR::VisitDecl(Decl *decl)
{
    CXXConstructorDecl *ctorDecl = dyn_cast<CXXConstructorDecl>(decl);
    CXXDestructorDecl *dtorDecl = dyn_cast<CXXDestructorDecl>(decl);
    if (ctorDecl == nullptr && dtorDecl == nullptr)
        return;

    Stmt *ctorOrDtorBody = ctorDecl ? ctorDecl->getBody() : dtorDecl->getBody();
    if (ctorOrDtorBody == nullptr)
        return;

    CXXRecordDecl *classDecl = ctorDecl ? ctorDecl->getParent() : dtorDecl->getParent();

    std::vector<Stmt*> processedStmts;
    SourceLocation loc = containsVirtualCall(classDecl, ctorOrDtorBody, processedStmts);
    if (loc.isValid()) {
        if (ctorDecl != nullptr) {
            emitWarning(decl->getLocStart(), "Calling pure virtual function in CTOR");
        } else {
            emitWarning(decl->getLocStart(), "Calling pure virtual function in DTOR");
        }
        emitWarning(loc, "Called here");
    }
}

SourceLocation VirtualCallsFromCTOR::containsVirtualCall(clang::CXXRecordDecl *classDecl, clang::Stmt *stmt, std::vector<Stmt*> &processedStmts)
{
    if (stmt == nullptr)
        return {};

    // already processed ? we don't want recurring calls
    if (clazy_std::contains(processedStmts, stmt))
        return {};

    processedStmts.push_back(stmt);

    std::vector<CXXMemberCallExpr*> memberCalls;
    HierarchyUtils::getChilds<CXXMemberCallExpr>(stmt, memberCalls);

    for (CXXMemberCallExpr *callExpr : memberCalls) {
        CXXMethodDecl *memberDecl = callExpr->getMethodDecl();
        if (!memberDecl || dyn_cast<CXXThisExpr>(callExpr->getImplicitObjectArgument()) == nullptr)
            continue;

        if (memberDecl->getParent() == classDecl) {
            if (memberDecl->isPure()) {
                return callExpr->getLocStart();
            } else {
                if (containsVirtualCall(classDecl, memberDecl->getBody(), processedStmts).isValid())
                    return callExpr->getLocStart();
            }
        }
    }

    return {};
}

REGISTER_CHECK("virtual-call-ctor", VirtualCallsFromCTOR, CheckLevel2)
