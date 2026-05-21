/*
    SPDX-FileCopyrightText: 2026 Author <your@email>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "modernize-list-initialization.h"
#include "HierarchyUtils.h"
#include "QtUtils.h"
#include "TypeUtils.h"
#include "Utils.h"

#include <clang/AST/AST.h>

using namespace clang;

ModernizeListInitialization::ModernizeListInitialization(const std::string &name, Options options)
    : CheckBase(name, options)
{
}

void ModernizeListInitialization::VisitDecl(clang::Decl *decl)
{
}

void ModernizeListInitialization::VisitStmt(clang::Stmt *stmt)
{
    if (auto expr = dyn_cast<DeclStmt>(stmt)) {
        if (auto varDecl = dyn_cast<VarDecl>(expr->getSingleDecl())) {
            if (auto *initExpr = dyn_cast<ExprWithCleanups>(varDecl->getInit())) {
                if (initExpr->getType().getAsString() == "QStringList") {
                    if (auto constructExpr = dyn_cast<CXXConstructExpr>(initExpr->getSubExpr()); constructExpr && constructExpr->getNumArgs() > 0) {
                        std::vector<std::string> replacementTexts;
                        auto opCall = dyn_cast<CXXOperatorCallExpr>(constructExpr->getArg(0)->IgnoreImplicit());

                        while (opCall && opCall->getNumArgs() > 0) {
                            auto charRange = Lexer::getAsCharRange(opCall->getArg(1)->getSourceRange(), sm(), lo());
                            replacementTexts.push_back(Lexer::getSourceText(charRange, sm(), lo()).str());
                            opCall = dyn_cast<CXXOperatorCallExpr>(opCall->getArg(0));
                        }

                        if (!replacementTexts.empty()) {
                            std::reverse(replacementTexts.begin(), replacementTexts.end());
                            std::string replacementText = "{";
                            for (auto str : replacementTexts) {
                                replacementText += str + ",";
                            }
                            replacementText[replacementText.length() - 1] = '}';
                            std::vector<FixItHint> fixes{FixItHint::CreateReplacement(initExpr->getSourceRange(), replacementText)};
                            emitWarning(initExpr->getBeginLoc(), "meh", fixes);
                        }
                    }
                }
            }
        }
    }
}
