/*
  This file is part of the clazy static checker.

    Copyright (C) 2016 Sergio Martins <smartins@kde.org>

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

#include "post-event.h"
#include "TypeUtils.h"
#include "StringUtils.h"

#include <clang/AST/Expr.h>
#include <clang/AST/Stmt.h>
#include <clang/Basic/LLVM.h>
#include <llvm/Support/Casting.h>

class ClazyContext;

using namespace clang;
using namespace std;


PostEvent::PostEvent(const std::string &name, ClazyContext *context)
    : CheckBase(name, context, Option_CanIgnoreIncludes)
{
}

void PostEvent::VisitStmt(clang::Stmt *stmt)
{
    auto callexpr = dyn_cast<CallExpr>(stmt);
    if (!callexpr)
        return;

    auto name = clazy::qualifiedMethodName(callexpr);

    const bool isPostEvent = name == "QCoreApplication::postEvent";
    const bool isSendEvent = name == "QCoreApplication::sendEvent";

    //if (!isPostEvent && !isSendEvent)
    // Send event has false-positives
    if (!isPostEvent)
        return;

    Expr *event = callexpr->getNumArgs() > 1 ? callexpr->getArg(1) : nullptr;
    if (!event || clazy::simpleTypeName(event->getType(), lo()) != "QEvent *")
        return;

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
