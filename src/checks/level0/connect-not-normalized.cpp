/*
    SPDX-FileCopyrightText: 2017 Klarälvdalens Datakonsult AB a KDAB Group company info@kdab.com
    SPDX-FileContributor: Sérgio Martins <sergio.martins@kdab.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
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

ConnectNotNormalized::ConnectNotNormalized(const std::string &name, ClazyContext *context)
    : CheckBase(name, context, Option_CanIgnoreIncludes)
{
}

void ConnectNotNormalized::VisitStmt(clang::Stmt *stmt)
{
    handleQ_ARG(stmt) || handleConnect(dyn_cast<CallExpr>(stmt));
}

bool ConnectNotNormalized::handleQ_ARG(clang::Stmt *stmt)
{
    if (auto *casted = dyn_cast<CallExpr>(stmt); casted && casted->getNumArgs() == 2) {
        if (auto *func = casted->getDirectCallee()) {
            const std::string retTypeName = func->getReturnType().getAsString(lo());
            if (retTypeName == "QMetaMethodArgument" || retTypeName == "QMetaMethodReturnArgument") {
                auto *literal = clazy::getFirstChildOfType2<StringLiteral>(casted->getArg(0));
                return literal ? checkNormalizedLiteral(literal, casted) : false;
            }
        }
    }
    auto *expr = dyn_cast<CXXConstructExpr>(stmt);
    if (!expr || expr->getNumArgs() != 2) {
        return false;
    }

    CXXConstructorDecl *ctor = expr->getConstructor();
    if (!ctor) {
        return false;
    }

    auto name = ctor->getNameAsString();
    if (name != "QArgument" && name != "QReturnArgument") {
        return false;
    }

    auto *sl = clazy::getFirstChildOfType2<clang::StringLiteral>(expr->getArg(0));
    return sl && checkNormalizedLiteral(sl, expr);
}
bool ConnectNotNormalized::checkNormalizedLiteral(clang::StringLiteral *sl, clang::Expr *expr)
{
    const std::string original = sl->getString().str();
    const std::string normalized = clazy::normalizedType(original.c_str());

    if (original == normalized) {
        return false;
    }

    emitWarning(expr, "Signature is not normalized. Use " + normalized + " instead of " + original);
    return true;
}

bool ConnectNotNormalized::handleConnect(CallExpr *callExpr)
{
    if (!callExpr) {
        return false;
    }

    FunctionDecl *func = callExpr->getDirectCallee();
    if (!func || func->getNumParams() != 1 || clazy::name(func) != "qFlagLocation") {
        return false;
    }

    {
        // Only warn in connect statements, not disconnect, since there there's no optimization in Qt's side
        auto *parentCallExpr = clazy::getFirstParentOfType<CallExpr>(m_context->parentMap, m_context->parentMap->getParent(callExpr), -1);
        if (!parentCallExpr) {
            return false;
        }

        FunctionDecl *parentFunc = parentCallExpr->getDirectCallee();
        if (!parentFunc || clazy::name(parentFunc) != "connect") {
            return false;
        }
    }

    Expr *arg1 = callExpr->getArg(0);
    auto *sl = clazy::getFirstChildOfType2<clang::StringLiteral>(arg1);
    if (!sl) {
        return false;
    }
    std::string original = sl->getString().str();
    std::string normalized = clazy::normalizedSignature(original.c_str());

    // discard the junk after '\0'
    normalized = std::string(normalized.c_str());
    original = std::string(original.c_str());

    if (original == normalized) {
        return false;
    }

    // Remove first digit
    normalized.erase(0, 1);
    original.erase(0, 1);

    emitWarning(clazy::getLocStart(callExpr), "Signature is not normalized. Use " + normalized + " instead of " + original);
    return true;
}
