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

#include "ruleoftwosoft.h"
#include "Utils.h"
#include "checkmanager.h"
#include "StringUtils.h"

#include <clang/AST/AST.h>
#include <clang/Lex/Lexer.h>

using namespace clang;
using namespace std;


RuleOfTwoSoft::RuleOfTwoSoft(const std::string &name, const clang::CompilerInstance &ci)
    : RuleOfBase(name, ci)
{
}

void RuleOfTwoSoft::VisitStmt(Stmt *s)
{
    if (auto op = dyn_cast<CXXOperatorCallExpr>(s)) {
        FunctionDecl *func = op->getDirectCallee();
        if (func && func->getNameAsString() == "operator=") {
            auto method = dyn_cast<CXXMethodDecl>(func);
            if (method && method->getParent()) {
                CXXRecordDecl *record = method->getParent();
                const bool hasCopyCtor = record->hasNonTrivialCopyConstructor();
                const bool hasCopyAssignOp = record->hasNonTrivialCopyAssignment();
                if (hasCopyCtor && !hasCopyAssignOp && !isBlacklisted(record)) {
                    string msg = "Using assign operator but class " + record->getQualifiedNameAsString() + " has copy-ctor but no assign operator";
                    emitWarning(s->getLocStart(), msg);
                }
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
                emitWarning(s->getLocStart(), msg);
            }
        }
    }
}

REGISTER_CHECK_WITH_FLAGS("rule-of-two-soft", RuleOfTwoSoft, CheckLevel1)
