/*
  This file is part of the clazy static checker.

  Copyright (C) 2020 Jesper K. Pedersen <jesper.pedersen@kdab.com>

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

#include "use-chrono-in-qtimer.h"
#include "HierarchyUtils.h"
#include <clang/AST/AST.h>

using namespace clang;
using namespace std;

static int unpackValue(clang::Expr* expr)
{
    auto value = dyn_cast<IntegerLiteral>(expr);
    if (value)
        return static_cast<int>(*value->getValue().getRawData());

    auto binaryOp = dyn_cast<BinaryOperator>(expr);
    if (!binaryOp)
        return -1;

    if (binaryOp->getOpcode() != BO_Mul)
        return -1;

    int left = unpackValue(binaryOp->getLHS());
    int right = unpackValue(binaryOp->getRHS());
    if (left == -1 || right == -1)
        return -1;

    return left * right;
}

void UseChronoInQTimer::warn(const clang::Stmt *stmt, int value)
{
    if (value == 0)
        return; // ignore zero times;

    std::string suggestion;
    if (value % (1000*3600) == 0)
        suggestion = std::to_string(value/1000/3600) + "h";
    else if (value % (1000*60) == 0)
        suggestion = std::to_string(value/1000/60) + "min";
    else if (value % 1000 == 0)
        suggestion = std::to_string(value/1000) + "s";
    else
        suggestion = std::to_string(value) + "ms";
    emitWarning(stmt, "make code more robust: use " + suggestion +" instead.");
}

static std::string functionName(CallExpr* callExpr)
{
    auto memberCall = clazy::getFirstChildOfType<MemberExpr>(callExpr);
    if (memberCall) {
        auto methodDecl = dyn_cast<CXXMethodDecl>(memberCall->getMemberDecl());
        if (!methodDecl)
            return {};
        return methodDecl->getQualifiedNameAsString();
    }

    FunctionDecl *fdecl = callExpr->getDirectCallee();
    if (fdecl)
        return fdecl->getQualifiedNameAsString();

    return {};
}

void UseChronoInQTimer::VisitStmt(clang::Stmt *stmt)
{
    auto callExpr = dyn_cast<CallExpr>(stmt);
    if (!callExpr)
        return;

    if (callExpr->getNumArgs() == 0)
        return; // start() doesn't take any arguments.

    const std::string name = functionName(callExpr);
    if ( name != "QTimer::setInterval" && name != "QTimer::start" && name != "QTimer::singleShot" )
        return;

    const int value = unpackValue(callExpr->getArg(0));
    if (value == -1)
        return;

    warn(callExpr->getArg(0), value);
}

