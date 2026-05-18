/*
    Copyright (C) 2026 Author <your@email>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "modernize-overloaded-signals.h"
#include "HierarchyUtils.h"
#include "QtUtils.h"
#include "TypeUtils.h"
#include "Utils.h"
#include "llvm/Support/Casting.h"

#include <clang/AST/AST.h>

using namespace clang;

ModernizeOverloadedSignals::ModernizeOverloadedSignals(const std::string &name, Options options)
    : CheckBase(name, options)
{
}

void ModernizeOverloadedSignals::VisitDecl(clang::Decl *decl)
{
}

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
    CXXMethodDecl *signalMethod = clazy::pmfFromConnect(call, 1, m_context->qtNamespace());
    if (!signalMethod) {
        return;
    }
    const std::string signalMethodName = signalMethod->getNameAsString();
    bool isMethodOverloaded = false;
    for (CXXMethodDecl *method : signalMethod->getParent()->methods()) {
        if (!isMethodOverloaded && method != signalMethod) {
            isMethodOverloaded = signalMethodName == method->getNameAsString();
        }
    }
    Expr *firstArg = call->getArg(1)->IgnoreImplicit();
    if (auto operatorCall = dyn_cast<CXXOperatorCallExpr>(firstArg); operatorCall && operatorCall->getNumArgs() > 1 && !isMethodOverloaded) {
        if (auto expr = dyn_cast<DeclRefExpr>(operatorCall->getArg(0)->IgnoreImplicit())) {
            if (expr->getType().getUnqualifiedType().getAsString().starts_with(qtNamespaced("QOverload"))) {
                std::vector<FixItHint> fixits{
                    FixItHint::CreateRemoval(SourceRange(operatorCall->getBeginLoc(), operatorCall->getArg(1)->getBeginLoc().getLocWithOffset(-1))),
                    FixItHint::CreateRemoval(SourceRange(operatorCall->getEndLoc(), operatorCall->getEndLoc())),
                };
                emitWarning(stmt->getBeginLoc(), "Unneeded qOverload", fixits);
            }
        }
    }
    if (auto callExpr = dyn_cast<CallExpr>(firstArg)) {
        if (auto calleeDecl = dyn_cast<CXXMethodDecl>(callExpr->getDirectCallee())) {
            if (calleeDecl->getNameAsString() == "of" && calleeDecl->getParent()->getNameAsString() == "QNonConstOverload") {
                if (isMethodOverloaded) {
                    std::vector<FixItHint> fixits{
                        FixItHint::CreateReplacement(SourceRange(callExpr->getBeginLoc(), callExpr->getBeginLoc().getLocWithOffset(1)), "qOverload"),
                        FixItHint::CreateRemoval(SourceRange(callExpr->getArg(0)->getBeginLoc().getLocWithOffset(-5), // Remove ::of part from source code
                                                             callExpr->getArg(0)->getBeginLoc().getLocWithOffset(-2))),
                    };
                    emitWarning(stmt->getBeginLoc(), "Use more concise qOverload", fixits);
                } else {
                    std::vector<FixItHint> fixits{
                        FixItHint::CreateRemoval(SourceRange(callExpr->getBeginLoc(), callExpr->getArg(0)->getBeginLoc().getLocWithOffset(-1))),
                        FixItHint::CreateRemoval(SourceRange(callExpr->getEndLoc(), callExpr->getEndLoc())),
                    };
                    emitWarning(stmt->getBeginLoc(), "Unneeded QOverload::of", fixits);
                }
            }
        }
    }
    if (auto castExpr = dyn_cast<CXXStaticCastExpr>(firstArg)) {
        auto subExpr = castExpr->getSubExpr();
        if (isMethodOverloaded) {
            std::string signalOverloadParams;
            for (auto p : signalMethod->parameters()) {
                if (!signalOverloadParams.empty()) {
                    signalOverloadParams += ", ";
                }
                signalOverloadParams += p->getType().getAsString();
            }
            std::vector<FixItHint> fixits{
                FixItHint::CreateReplacement(SourceRange(castExpr->getBeginLoc(), subExpr->getBeginLoc().getLocWithOffset(-2)),
                                             "qOverload<" + signalOverloadParams + ">"),
            };
            emitWarning(stmt->getBeginLoc(), "Unneeded QOverload::of", fixits);
        } else {
            std::vector<FixItHint> fixits{
                FixItHint::CreateRemoval(SourceRange(castExpr->getBeginLoc(), subExpr->getBeginLoc().getLocWithOffset(-1))),
                FixItHint::CreateRemoval(SourceRange(castExpr->getEndLoc(), castExpr->getEndLoc())),
            };
            emitWarning(stmt->getBeginLoc(), "Unneeded QOverload::of", fixits);
        }
    }
}
