/*
    SPDX-FileCopyrightText: 2026 Author <your@email>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "modernize-list-initialization.h"
#include "QtUtils.h"
#include "TypeUtils.h"
#include "Utils.h"
#include "clang/AST/ParentMap.h"

#include <algorithm>
#include <clang/AST/AST.h>

using namespace clang;

void ModernizeListInitialization::VisitStmt(clang::Stmt *stmt)
{
    if (auto expr = dyn_cast<DeclStmt>(stmt)) {
        if (auto varDecl = dyn_cast<VarDecl>(expr->getSingleDecl())) {
            if (varDecl->hasInit()) {
                if (auto *initExpr = dyn_cast<ExprWithCleanups>(varDecl->getInit())) {
                    const std::string name = initExpr->getType().getUnqualifiedType().getAsString();
                    if (name == "QStringList" || name.starts_with("QList")) {
                        if (auto constructExpr = dyn_cast<CXXConstructExpr>(initExpr->getSubExpr()); constructExpr && constructExpr->getNumArgs() > 0) {
                            auto exprArg = constructExpr->getArg(0)->IgnoreImplicit();
                            // Resolve any ParenExpr that might be in between
                            if (auto parenExpr = dyn_cast<ParenExpr>(exprArg)) {
                                exprArg = parenExpr->getSubExpr();
                            }
                            auto operatorCall = dyn_cast<CXXOperatorCallExpr>(exprArg);
                            if (operatorCall && operatorCall->getOperator() == OverloadedOperatorKind::OO_LessLess) {
                                SourceLocation afterName = Lexer::getLocForEndOfToken(varDecl->getLocation(), 0, sm(), lo());
                                checkOperatorCallListInitialization(SourceRange(afterName, initExpr->getEndLoc()), operatorCall);
                                m_alreadyCheckedOperatorCalls.push_back(operatorCall);
                            }
                        }
                    }
                }
            }
        }
    }

    if (auto *topLevelOptaratorCall = dyn_cast<CXXOperatorCallExpr>(stmt);
        topLevelOptaratorCall && topLevelOptaratorCall->getOperator() == OverloadedOperatorKind::OO_LessLess) {
        if (std::ranges::find(m_alreadyCheckedOperatorCalls, topLevelOptaratorCall) != m_alreadyCheckedOperatorCalls.end()) {
            return;
        }
        std::string name = topLevelOptaratorCall->getType().getAsString();
        if (name != "QStringList" && !name.starts_with("QList")) {
            return;
        }
        auto parent = m_context->parentMap->getParent(topLevelOptaratorCall);
        if (!isa<ImplicitCastExpr>(parent)) {
            return;
        }
        if (auto bla = dyn_cast<MaterializeTemporaryExpr>(topLevelOptaratorCall->getArg(0))) {
            if (auto bla2 = dyn_cast<CXXBindTemporaryExpr>(bla->getSubExpr())) {
                if (isa<CallExpr>(bla2->getSubExpr())) {
                    return; // A function has produced this list - we are not initializing it!
                }
            }
        }
        checkOperatorCallListInitialization({parent->getBeginLoc(), topLevelOptaratorCall->getEndLoc()}, topLevelOptaratorCall);
    }
}

void ModernizeListInitialization::checkOperatorCallListInitialization(clang::SourceRange fixitSourceRange, clang::CXXOperatorCallExpr *operatorCall)
{
    std::vector<std::string> replacementTexts;
    auto opCall = operatorCall;
    while (opCall && opCall->getNumArgs() > 0) {
        auto firstArg = opCall->getArg(1);
        std::string sourceText = getSourceText(firstArg).str();
        if (auto token = Lexer::findNextToken(firstArg->getEndLoc(), sm(), lo(), true); token.has_value() && token.value().is(tok::TokenKind::comment)) {
            std::string comment = getSourceText(CharSourceRange::getTokenRange(token->getLocation())).str();
            if (comment.starts_with("//")) {
                sourceText += ", " + comment + "\n";
            } else {
                sourceText += +" " + comment + ", ";
            }
        } else {
            sourceText += ", ";
        }
        replacementTexts.push_back(sourceText);
        opCall = dyn_cast<CXXOperatorCallExpr>(opCall->getArg(0));
    }
    if (replacementTexts.empty()) {
        return;
    }
    std::reverse(replacementTexts.begin(), replacementTexts.end());

    std::string replacementText = "{ ";
    for (auto str : replacementTexts) {
        replacementText += str;
    }
    if (replacementText[replacementText.length() - 1] == '\n') {
        replacementText += "}";
    } else {
        replacementText[replacementText.length() - 1] = '}';
    }

    std::vector<FixItHint> fixes{FixItHint::CreateReplacement(fixitSourceRange, replacementText)};
    emitWarning(fixitSourceRange.getBegin(), "Use initializer-list syntax", fixes);
}
