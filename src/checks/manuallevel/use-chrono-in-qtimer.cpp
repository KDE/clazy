/*
    SPDX-FileCopyrightText: 2020 Jesper K. Pedersen <jesper.pedersen@kdab.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "use-chrono-in-qtimer.h"
#include "ClazyContext.h"
#include "FixItUtils.h"
#include "HierarchyUtils.h"
#include "PreProcessorVisitor.h"
#include <clang/AST/AST.h>

using namespace clang;

UseChronoInQTimer::UseChronoInQTimer(const std::string &name)
    : CheckBase(name, Option_CanIgnoreIncludes)
{
}

static int unpackValue(clang::Expr *expr)
{
    auto *value = dyn_cast<IntegerLiteral>(expr);
    if (value) {
        return static_cast<int>(*value->getValue().getRawData());
    }

    auto *binaryOp = dyn_cast<BinaryOperator>(expr);
    if (!binaryOp) {
        return -1;
    }

    int left = unpackValue(binaryOp->getLHS());
    int right = unpackValue(binaryOp->getRHS());
    if (left == -1 || right == -1) {
        return -1;
    }

    if (binaryOp->getOpcode() == BO_Mul) {
        return left * right;
    }

    if (binaryOp->getOpcode() == BO_Div) {
        return left / right;
    }

    return -1;
}

void UseChronoInQTimer::warn(const clang::Stmt *stmt, int value)
{
    if (value == 0) {
        return; // ignore zero times;
    }

    std::string suggestion;
    if (value % (1000 * 3600) == 0) {
        suggestion = std::to_string(value / 1000 / 3600) + "h";
    } else if (value % (1000 * 60) == 0) {
        suggestion = std::to_string(value / 1000 / 60) + "min";
    } else if (value % 1000 == 0) {
        suggestion = std::to_string(value / 1000) + "s";
    } else {
        suggestion = std::to_string(value) + "ms";
    }

    std::vector<FixItHint> fixits;
    fixits.push_back(FixItHint::CreateReplacement(stmt->getSourceRange(), suggestion));

    if (!m_hasInsertedInclude && m_context->preprocessorVisitor && !m_context->preprocessorVisitor->hasInclude("chrono", true)) {
        fixits.push_back(clazy::createInsertion(m_context->preprocessorVisitor->endOfIncludeSection(),
                                                "\n"
                                                "#include <chrono>\n\n"
                                                "using namespace std::chrono_literals;"));
    }
    m_hasInsertedInclude = true;

    emitWarning(stmt->getBeginLoc(), "make code more robust: use " + suggestion + " instead.", fixits);
}

static std::string functionName(CallExpr *callExpr)
{
    auto *memberCall = clazy::getFirstChildOfType<MemberExpr>(callExpr);
    if (memberCall) {
        auto *methodDecl = dyn_cast<CXXMethodDecl>(memberCall->getMemberDecl());
        if (!methodDecl) {
            return {};
        }
        return methodDecl->getQualifiedNameAsString();
    }

    FunctionDecl *fdecl = callExpr->getDirectCallee();
    if (fdecl) {
        return fdecl->getQualifiedNameAsString();
    }

    return {};
}

void UseChronoInQTimer::VisitStmt(clang::Stmt *stmt)
{
    auto *callExpr = dyn_cast<CallExpr>(stmt);
    if (!callExpr) {
        return;
    }

    if (callExpr->getNumArgs() == 0) {
        return; // start() doesn't take any arguments.
    }

    const std::string name = functionName(callExpr);
    if (name != "QTimer::setInterval" && name != "QTimer::start" && name != "QTimer::singleShot") {
        return;
    }

    const int value = unpackValue(callExpr->getArg(0));
    if (value == -1) {
        return;
    }

    warn(callExpr->getArg(0), value);
}
