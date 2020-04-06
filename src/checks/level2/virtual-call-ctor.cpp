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

#include "virtual-call-ctor.h"
#include "HierarchyUtils.h"
#include "SourceCompatibilityHelpers.h"
#include "clazy_stl.h"
#include "ClazyContext.h"

#include <clang/AST/DeclCXX.h>
#include <clang/AST/DeclBase.h>
#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/Stmt.h>
#include <clang/Basic/LLVM.h>
#include <clang/Basic/SourceLocation.h>
#include <llvm/Support/Casting.h>

class ClazyContext;

using namespace std;
using namespace clang;

namespace clazy {
void getChildsIgnoreLambda(clang::Stmt *stmt, std::vector<CXXMemberCallExpr*> &result_list, int depth = -1)
{
    if (!stmt || dyn_cast<LambdaExpr>(stmt))
        return;

    auto cexpr = llvm::dyn_cast<CXXMemberCallExpr>(stmt);
    if (cexpr)
        result_list.push_back(cexpr);

    if (depth > 0 || depth == -1) {
        if (depth > 0)
            --depth;
        for (auto child : stmt->children()) {
            getChildsIgnoreLambda(child, result_list, depth);
        }
    }
}
}

VirtualCallCtor::VirtualCallCtor(const std::string &name, ClazyContext *context)
    : CheckBase(name, context)
{
}

void VirtualCallCtor::VisitDecl(Decl *decl)
{
    auto ctorDecl = dyn_cast<CXXConstructorDecl>(decl);
    auto dtorDecl = dyn_cast<CXXDestructorDecl>(decl);
    if (!ctorDecl && !dtorDecl)
        return;

    Stmt *ctorOrDtorBody = ctorDecl ? ctorDecl->getBody() : dtorDecl->getBody();
    if (!ctorOrDtorBody)
        return;

    CXXRecordDecl *classDecl = ctorDecl ? ctorDecl->getParent() : dtorDecl->getParent();

    std::vector<Stmt*> processedStmts;
    SourceLocation loc = containsVirtualCall(classDecl, ctorOrDtorBody, processedStmts);
    if (loc.isValid()) {
        if (ctorDecl) {
            emitWarning(clazy::getLocStart(decl), "Calling pure virtual function in CTOR");
        } else {
            emitWarning(clazy::getLocStart(decl), "Calling pure virtual function in DTOR");
        }
        emitWarning(loc, "Called here");
    }
}

SourceLocation VirtualCallCtor::containsVirtualCall(clang::CXXRecordDecl *classDecl, clang::Stmt *stmt,
                                                    std::vector<Stmt*> &processedStmts)
{
    if (!stmt)
        return {};

    // already processed ? we don't want recurring calls
    if (clazy::contains(processedStmts, stmt))
        return {};

    processedStmts.push_back(stmt);

    std::vector<CXXMemberCallExpr*> memberCalls;

    // Ignore lambdas, as they are usually used in connects. Might introduce true-negatives. Possible solution would be to check if the lambda is in a connect, timer, invokeMethod, etc.
    clazy::getChildsIgnoreLambda(stmt, memberCalls);

    for (CXXMemberCallExpr *callExpr : memberCalls) {
        CXXMethodDecl *memberDecl = callExpr->getMethodDecl();
        if (!memberDecl || !isa<CXXThisExpr>(callExpr->getImplicitObjectArgument()))
            continue;

        if (memberDecl->getParent() == classDecl) {
            if (memberDecl->isPure()) {
                return clazy::getLocStart(callExpr);
            } else {
                if (containsVirtualCall(classDecl, memberDecl->getBody(), processedStmts).isValid())
                    return clazy::getLocStart(callExpr);
            }
        }
    }

    return {};
}
