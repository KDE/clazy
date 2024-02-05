/*
    SPDX-FileCopyrightText: 2016 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "qstring-insensitive-allocation.h"
#include "SourceCompatibilityHelpers.h"
#include "StringUtils.h"
#include "Utils.h"
#include "clazy_stl.h"

#include <clang/AST/Expr.h>
#include <clang/AST/Stmt.h>
#include <clang/Basic/LLVM.h>
#include <llvm/Support/Casting.h>

#include <vector>

class ClazyContext;
namespace clang
{
class FunctionDecl;
} // namespace clang

using namespace clang;

QStringInsensitiveAllocation::QStringInsensitiveAllocation(const std::string &name, ClazyContext *context)
    : CheckBase(name, context, Option_CanIgnoreIncludes)
{
}

static bool isInterestingCall1(CallExpr *call)
{
    FunctionDecl *func = call->getDirectCallee();
    if (!func) {
        return false;
    }

    static const std::vector<std::string> methods = {"QString::toUpper", "QString::toLower"};
    return clazy::contains(methods, clazy::qualifiedMethodName(func));
}

static bool isInterestingCall2(CallExpr *call)
{
    FunctionDecl *func = call->getDirectCallee();
    if (!func) {
        return false;
    }

    static const std::vector<std::string> methods = {"QString::endsWith", "QString::startsWith", "QString::contains", "QString::compare"};
    return clazy::contains(methods, clazy::qualifiedMethodName(func));
}

void QStringInsensitiveAllocation::VisitStmt(clang::Stmt *stmt)
{
    std::vector<CallExpr *> calls = Utils::callListForChain(dyn_cast<CallExpr>(stmt));
    if (calls.size() < 2) {
        return;
    }

    CallExpr *call1 = calls[calls.size() - 1];
    CallExpr *call2 = calls[calls.size() - 2];

    if (!isInterestingCall1(call1) || !isInterestingCall2(call2)) {
        return;
    }

    emitWarning(stmt->getBeginLoc(), "unneeded allocation");
}
