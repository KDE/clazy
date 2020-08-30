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
#include "ClazyContext.h"
#include "FixItUtils.h"
#include "PreProcessorVisitor.h"

using namespace clang;
using namespace std;


UseChronoInQTimer::UseChronoInQTimer(const std::string &name, ClazyContext *context)
    : CheckBase(name, context, Option_CanIgnoreIncludes)
{
    context->enablePreprocessorVisitor();
}

static int unpackValue(clang::Expr* expr)
{
    auto value = dyn_cast<IntegerLiteral>(expr);
    if (value)
        return static_cast<int>(*value->getValue().getRawData());

    auto binaryOp = dyn_cast<BinaryOperator>(expr);
    if (!binaryOp)
        return -1;

    int left = unpackValue(binaryOp->getLHS());
    int right = unpackValue(binaryOp->getRHS());
    if (left == -1 || right == -1)
        return -1;

    if (binaryOp->getOpcode() == BO_Mul)
        return left * right;

    if (binaryOp->getOpcode() == BO_Div)
        return left / right;

    return -1;
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

    vector<FixItHint> fixits;
#if LLVM_VERSION_MAJOR >= 11 // LLVM < 11 has a problem with \n in the yaml replacements file
    fixits.push_back(FixItHint::CreateReplacement(stmt->getSourceRange(), suggestion));

    if (!m_hasInsertedInclude && !m_context->preprocessorVisitor->hasInclude("chrono", true)) {
        fixits.push_back(clazy::createInsertion(m_context->preprocessorVisitor->endOfIncludeSection(),
                                                "\n"
                                                "#include <chrono>\n\n"
                                                "using namespace std::chrono_literals;"));
    }
#endif
    m_hasInsertedInclude = true;

    emitWarning(clazy::getLocStart(stmt), "make code more robust: use " + suggestion +" instead.", fixits);
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

