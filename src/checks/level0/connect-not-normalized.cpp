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

#include "connect-not-normalized.h"
#include "ClazyContext.h"
#include "HierarchyUtils.h"
#include "NormalizedSignatureUtils.h"
#include "SourceCompatibilityHelpers.h"
#include "StringUtils.h"

#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/ParentMap.h>
#include <clang/AST/Stmt.h>
#include <clang/Basic/LLVM.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Casting.h>

using namespace clang;
using namespace std;

ConnectNotNormalized::ConnectNotNormalized(const std::string &name, ClazyContext *context)
    : CheckBase(name, context, Option_CanIgnoreIncludes)
{
}

void ConnectNotNormalized::VisitStmt(clang::Stmt *stmt)
{
    if (handleQ_ARG(dyn_cast<CXXConstructExpr>(stmt)))
        return;

    handleConnect(dyn_cast<CallExpr>(stmt));
}

bool ConnectNotNormalized::handleQ_ARG(CXXConstructExpr *expr)
{
    if (!expr || expr->getNumArgs() != 2)
        return false;

    CXXConstructorDecl *ctor = expr->getConstructor();
    if (!ctor)
        return false;

    auto name = ctor->getNameAsString();
    if (name != "QArgument" && name != "QReturnArgument")
        return false;

    StringLiteral *sl = clazy::getFirstChildOfType2<StringLiteral>(expr->getArg(0));
    if (!sl)
        return false;

    const std::string original = sl->getString().str();
    const std::string normalized = clazy::normalizedType(original.c_str());

    if (original == normalized)
        return false;

    emitWarning(expr, "Signature is not normalized. Use " + normalized + " instead of " + original);
    return true;
}

bool ConnectNotNormalized::handleConnect(CallExpr *callExpr)
{
    if (!callExpr)
        return false;

    FunctionDecl *func = callExpr->getDirectCallee();
    if (!func || func->getNumParams() != 1 || clazy::name(func) != "qFlagLocation")
        return false;

    {
        // Only warn in connect statements, not disconnect, since there there's no optimization in Qt's side
        auto parentCallExpr = clazy::getFirstParentOfType<CallExpr>(m_context->parentMap,
                                                                    m_context->parentMap->getParent(callExpr), -1);
        if (!parentCallExpr)
            return false;

        FunctionDecl *parentFunc = parentCallExpr->getDirectCallee();
        if (!parentFunc || clazy::name(parentFunc) != "connect")
            return false;
    }

    Expr *arg1 = callExpr->getArg(0);
    StringLiteral *sl = clazy::getFirstChildOfType2<StringLiteral>(arg1);
    if (!sl)
        return false;
    std::string original = sl->getString().str();
    std::string normalized = clazy::normalizedSignature(original.c_str());

    // discard the junk after '\0'
    normalized = string(normalized.c_str());
    original = string(original.c_str());

    if (original == normalized)
        return false;

    // Remove first digit
    normalized.erase(0, 1);
    original.erase(0, 1);

    emitWarning(clazy::getLocStart(callExpr), "Signature is not normalized. Use " + normalized + " instead of " + original);
    return true;
}
