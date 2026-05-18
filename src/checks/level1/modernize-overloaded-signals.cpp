/*
    SPDX-FileCopyrightText: 2026 Alexander Lohnau <alexander.lohnau@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "modernize-overloaded-signals.h"
#include "QtUtils.h"
#include "TypeUtils.h"
#include "Utils.h"

#include <clang/AST/AST.h>

using namespace clang;

void ModernizeOverloadedSignals::VisitStmt(clang::Stmt *stmt)
{
    auto *call = dyn_cast<CallExpr>(stmt);
    const AccessSpecifierManager *accessSpecifierManager = m_context->accessSpecifierManager;
    if (!call || !accessSpecifierManager) {
        return;
    }

    FunctionDecl *func = call->getDirectCallee();
    if (!clazy::isConnect(func, m_context->qtNamespace()) || !clazy::connectHasPMFStyle(func)) {
        return;
    }
    if (CXXMethodDecl *signalMethod = clazy::pmfFromConnect(call, 1, m_context->qtNamespace())) {
        checkConnectArg(call, signalMethod, 1);
    }

    const int numArgToCheck = call->getNumArgs() > 3 ? 3 : 2;
    if (CXXMethodDecl *slotMethod = clazy::pmfFromConnect(call, numArgToCheck, m_context->qtNamespace())) {
        checkConnectArg(call, slotMethod, numArgToCheck);
    }
}

void ModernizeOverloadedSignals::checkConnectArg(CallExpr *call, CXXMethodDecl *pmfFromConnect, int numArgToCheck)
{
    const std::string pmfMethodName = pmfFromConnect->getNameAsString();
    bool isMethodOverloaded = false;
    for (CXXMethodDecl *method : pmfFromConnect->getParent()->methods()) {
        if (!isMethodOverloaded && method != pmfFromConnect) {
            isMethodOverloaded = pmfMethodName == method->getNameAsString();
        }
    }
    Expr *argToCheck = call->getArg(numArgToCheck)->IgnoreImplicit();
    if (auto operatorCall = dyn_cast<CXXOperatorCallExpr>(argToCheck); operatorCall && operatorCall->getNumArgs() > 1 && !isMethodOverloaded) {
        if (auto expr = dyn_cast<DeclRefExpr>(operatorCall->getArg(0)->IgnoreImplicit())) {
            if (expr->getType().getUnqualifiedType().getAsString().starts_with(qtNamespaced("QOverload"))) {
                std::vector<FixItHint> fixits{
                    FixItHint::CreateRemoval(SourceRange(operatorCall->getBeginLoc(), operatorCall->getArg(1)->getBeginLoc().getLocWithOffset(-1))),
                    FixItHint::CreateRemoval(SourceRange(operatorCall->getEndLoc(), operatorCall->getEndLoc())),
                };
                emitWarning(call->getBeginLoc(), "Unneeded qOverload", fixits);
            }
        }
    }
    if (auto callExpr = dyn_cast<CallExpr>(argToCheck)) {
        if (auto calleeDecl = dyn_cast<CXXMethodDecl>(callExpr->getDirectCallee())) {
            if (calleeDecl->getNameAsString() == "of" && calleeDecl->getParent()->getNameAsString() == "QNonConstOverload") {
                if (isMethodOverloaded) {
                    std::vector<FixItHint> fixits{
                        FixItHint::CreateReplacement(SourceRange(callExpr->getBeginLoc(), callExpr->getBeginLoc().getLocWithOffset(1)), "qOverload"),
                        FixItHint::CreateRemoval(SourceRange(callExpr->getArg(0)->getBeginLoc().getLocWithOffset(-5), // Remove ::of part from source code
                                                             callExpr->getArg(0)->getBeginLoc().getLocWithOffset(-2))),
                    };
                    emitWarning(call->getBeginLoc(), "Use more concise qOverload", fixits);
                } else {
                    std::vector<FixItHint> fixits{
                        FixItHint::CreateRemoval(SourceRange(callExpr->getBeginLoc(), callExpr->getArg(0)->getBeginLoc().getLocWithOffset(-1))),
                        FixItHint::CreateRemoval(SourceRange(callExpr->getEndLoc(), callExpr->getEndLoc())),
                    };
                    emitWarning(call->getBeginLoc(), "Unneeded QOverload::of", fixits);
                }
            }
        }
    }
    if (auto castExpr = dyn_cast<CXXStaticCastExpr>(argToCheck)) {
        auto subExpr = castExpr->getSubExpr();
        if (isMethodOverloaded) {
            std::string signalOverloadParams;
            for (auto p : pmfFromConnect->parameters()) {
                if (!signalOverloadParams.empty()) {
                    signalOverloadParams += ", ";
                }
                signalOverloadParams += p->getType().getAsString();
            }
            std::vector<FixItHint> fixits{
                FixItHint::CreateReplacement(SourceRange(castExpr->getBeginLoc(), subExpr->getBeginLoc().getLocWithOffset(-2)),
                                             "qOverload<" + signalOverloadParams + ">"),
            };
            emitWarning(call->getBeginLoc(), "Unneeded QOverload::of", fixits);
        } else {
            std::vector<FixItHint> fixits{
                FixItHint::CreateRemoval(SourceRange(castExpr->getBeginLoc(), subExpr->getBeginLoc().getLocWithOffset(-1))),
                FixItHint::CreateRemoval(SourceRange(castExpr->getEndLoc(), castExpr->getEndLoc())),
            };
            emitWarning(call->getBeginLoc(), "Unneeded QOverload::of", fixits);
        }
    }
}
