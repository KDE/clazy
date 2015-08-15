/**********************************************************************
**  Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
**  Author: Sérgio Martins <sergio.martins@kdab.com>
**
** This file may be distributed and/or modified under the terms of the
** GNU Lesser General Public License version 2.1 and version 3 as published by the
** Free Software Foundation and appearing in the file LICENSE.LGPL.txt included.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**********************************************************************/

#include "virtualcallsfromctor.h"
#include "Utils.h"

#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>

using namespace std;
using namespace clang;

VirtualCallsFromCTOR::VirtualCallsFromCTOR(clang::CompilerInstance &ci)
    : CheckBase(ci)
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
    if (containsVirtualCall(classDecl, ctorOrDtorBody, processedStmts)) {
        if (ctorDecl != nullptr)
            emitWarning(ctorOrDtorBody->getLocStart(), "Calling pure virtual function in CTOR");
        else
            emitWarning(ctorOrDtorBody->getLocStart(), "Calling pure virtual function in DTOR");
    }
}

std::string VirtualCallsFromCTOR::name() const
{
    return "virtual-call-ctor";
}

bool VirtualCallsFromCTOR::containsVirtualCall(clang::CXXRecordDecl *classDecl, clang::Stmt *stmt, std::vector<Stmt*> &processedStmts) const
{
    if (stmt == nullptr)
        return false;

    // already processed ? we don't want recurring calls
    if (std::find(processedStmts.cbegin(), processedStmts.cend(), stmt) != processedStmts.cend())
        return false;

    processedStmts.push_back(stmt);

    std::vector<CXXMemberCallExpr*> memberCalls;
    Utils::getChilds2<CXXMemberCallExpr>(stmt, memberCalls);

    for (CXXMemberCallExpr *callExpr : memberCalls) {
        CXXMethodDecl *memberDecl = callExpr->getMethodDecl();
        if (memberDecl == nullptr || dyn_cast<CXXThisExpr>(callExpr->getImplicitObjectArgument()) == nullptr)
            continue;

        if (memberDecl->getParent() == classDecl) {
            if (memberDecl->isPure()) {
                emitWarning(callExpr->getLocStart(), "Called here");
                return true;
            } else {
                if (containsVirtualCall(classDecl, memberDecl->getBody(), processedStmts))
                    return true;
            }
        }
    }

    return false;
}
