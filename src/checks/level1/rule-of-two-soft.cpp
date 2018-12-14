/*
    This file is part of the clazy static checker.

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

#include "rule-of-two-soft.h"
#include "SourceCompatibilityHelpers.h"

#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/Stmt.h>
#include <clang/Basic/LLVM.h>
#include <llvm/Support/Casting.h>

class ClazyContext;

using namespace clang;
using namespace std;


RuleOfTwoSoft::RuleOfTwoSoft(const std::string &name, ClazyContext *context)
    : RuleOfBase(name, context)
{
}

void RuleOfTwoSoft::VisitStmt(Stmt *s)
{
    if (auto op = dyn_cast<CXXOperatorCallExpr>(s)) {
        FunctionDecl *func = op->getDirectCallee();
        auto method = func ? dyn_cast<CXXMethodDecl>(func) : nullptr;
        if (method && method->getParent() &&  method->isCopyAssignmentOperator()) {
            CXXRecordDecl *record = method->getParent();
            const bool hasCopyCtor = record->hasNonTrivialCopyConstructor();
            const bool hasCopyAssignOp = record->hasNonTrivialCopyAssignment();
            if (hasCopyCtor && !hasCopyAssignOp && !isBlacklisted(record)) {
                string msg = "Using assign operator but class " + record->getQualifiedNameAsString() + " has copy-ctor but no assign operator";
                emitWarning(clazy::getLocStart(s), msg);
            }
        }
    } else if (auto ctorExpr = dyn_cast<CXXConstructExpr>(s)) {
        CXXConstructorDecl *ctorDecl = ctorExpr->getConstructor();
        CXXRecordDecl *record = ctorDecl->getParent();
        if (ctorDecl->isCopyConstructor() && record) {
            const bool hasCopyCtor = record->hasNonTrivialCopyConstructor();
            const bool hasCopyAssignOp = record->hasNonTrivialCopyAssignment();
            if (!hasCopyCtor && hasCopyAssignOp && !isBlacklisted(record)) {
                string msg = "Using copy-ctor but class " + record->getQualifiedNameAsString() + " has a trivial copy-ctor but non trivial assign operator";
                emitWarning(clazy::getLocStart(s), msg);
            }
        }
    }
}
