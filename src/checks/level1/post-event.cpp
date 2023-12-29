/*
    SPDX-FileCopyrightText: 2016 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "post-event.h"
#include "StringUtils.h"
#include "TypeUtils.h"

#include <clang/AST/Expr.h>
#include <clang/AST/Stmt.h>
#include <clang/Basic/LLVM.h>
#include <llvm/Support/Casting.h>

class ClazyContext;

using namespace clang;

PostEvent::PostEvent(const std::string &name, ClazyContext *context)
    : CheckBase(name, context, Option_CanIgnoreIncludes)
{
}

void PostEvent::VisitStmt(clang::Stmt *stmt)
{
    auto *callexpr = dyn_cast<CallExpr>(stmt);
    if (!callexpr) {
        return;
    }

    auto name = clazy::qualifiedMethodName(callexpr);

    const bool isPostEvent = name == "QCoreApplication::postEvent";
    const bool isSendEvent = name == "QCoreApplication::sendEvent";

    // if (!isPostEvent && !isSendEvent)
    //  Send event has false-positives
    if (!isPostEvent) {
        return;
    }

    Expr *event = callexpr->getNumArgs() > 1 ? callexpr->getArg(1) : nullptr;
    if (!event || clazy::simpleTypeName(event->getType(), lo()) != "QEvent *") {
        return;
    }

    bool isStack = false;
    bool isHeap = false;
    clazy::heapOrStackAllocated(event, "QEvent", lo(), isStack, isHeap);

    if (isStack || isHeap) {
        if (isSendEvent && isHeap) {
            emitWarning(stmt, "Events passed to sendEvent should be stack allocated");
        } else if (isPostEvent && isStack) {
            emitWarning(stmt, "Events passed to postEvent should be heap allocated");
        }
    } else {
        // It's something else, like an rvalue, ignore it
    }
}
