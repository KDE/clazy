/*
    SPDX-FileCopyrightText: 2017 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "install-event-filter.h"
#include "HierarchyUtils.h"
#include "TypeUtils.h"
#include "Utils.h"

#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/Stmt.h>
#include <clang/Basic/LLVM.h>
#include <llvm/Support/Casting.h>

using namespace clang;

InstallEventFilter::InstallEventFilter(const std::string &name, ClazyContext *context)
    : CheckBase(name, context, Option_CanIgnoreIncludes)
{
}

void InstallEventFilter::VisitStmt(clang::Stmt *stmt)
{
    auto *memberCallExpr = dyn_cast<CXXMemberCallExpr>(stmt);
    if (!memberCallExpr || memberCallExpr->getNumArgs() != 1) {
        return;
    }

    FunctionDecl *func = memberCallExpr->getDirectCallee();
    if (!func || func->getQualifiedNameAsString() != "QObject::installEventFilter") {
        return;
    }

    Expr *expr = memberCallExpr->getImplicitObjectArgument();
    if (!expr) {
        return;
    }

    if (Stmt *firstChild = clazy::getFirstChildAtDepth(expr, 1); !firstChild || !isa<CXXThisExpr>(firstChild)) {
        return;
    }

    Expr *arg1 = memberCallExpr->getArg(0);
    arg1 = arg1 ? arg1->IgnoreCasts() : nullptr;

    const CXXRecordDecl *record = clazy::typeAsRecord(arg1);
    auto methods = Utils::methodsFromString(record, "eventFilter");

    for (auto *method : methods) {
        if (method->getQualifiedNameAsString() != "QObject::eventFilter") { // It overrides it, probably on purpose then, don't warn.
            return;
        }
    }

    stmt->getBeginLoc().dump(sm());

    emitWarning(stmt->getBeginLoc(), "'this' should usually be the filter object, not the monitored one.");
}
