/*
    SPDX-FileCopyrightText: 2024 Alexander Lohnau <alexander.lohnau@gmx.de>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "used-qunused-variable.h"
#include "ClazyContext.h"
#include "MacroUtils.h"
#include "PreProcessorVisitor.h"
#include "TypeUtils.h"
#include "Utils.h"
#include "clang/AST/Decl.h"
#include "clang/AST/RecursiveASTVisitor.h"

#include <clang/AST/AST.h>

using namespace clang;

class ParameterUsageVisitor : public RecursiveASTVisitor<ParameterUsageVisitor>
{
public:
    explicit ParameterUsageVisitor(const ParmVarDecl *param)
        : param(param)
    {
    }

    bool VisitStmt(Stmt *stmt)
    {
        if (checkUsage(stmt)) {
            paramUsages.push_back(stmt);
        }
        return true;
    }

    std::vector<Stmt *> paramUsages;
    Stmt *qunusedParamUsage = nullptr;

private:
    const ParmVarDecl *param;

    bool checkUsage(Stmt *S, Stmt *parentStmt = nullptr)
    {
        if (!S)
            return false;
        if (DeclRefExpr *DRE = dyn_cast<DeclRefExpr>(S)) {
            if (DRE->getDecl() == param) {
                return true;
            }
        }

        if (CompoundStmt *cs = dyn_cast<CompoundStmt>(S)) {
            for (auto *child : cs->children()) {
                if (auto *castExpr = dyn_cast<CastExpr>(child); castExpr && castExpr->getType().getAsString() == "void") {
                    for (auto *possibleDeclRef : castExpr->children()) {
                        if (DeclRefExpr *declRef = dyn_cast<DeclRefExpr>(possibleDeclRef)) {
                            if (declRef->getDecl() == param) {
                                // We found a void cast
                                qunusedParamUsage = possibleDeclRef;
                            }
                        }
                    }
                }
                if (checkUsage(child, cs)) {
                    return true;
                }
            }
        }

        return false;
    }
};

UsedQUnusedVariable::UsedQUnusedVariable(const std::string &name, ClazyContext *context)
    : CheckBase(name, context)
{
}

void UsedQUnusedVariable::VisitDecl(clang::Decl *decl)
{
    const auto *fncDecl = dyn_cast<FunctionDecl>(decl);
    if (!fncDecl) {
        return;
    }
    for (auto *param : fncDecl->parameters()) {
        if (param->isUsed()) {
            ParameterUsageVisitor visitor(param);

            // Visit the compound statement (the function body) to find usages
            visitor.TraverseStmt(fncDecl->getBody());

            if (visitor.paramUsages.size() > 1 && visitor.qunusedParamUsage) {
                if (auto beginLoc = visitor.qunusedParamUsage->getBeginLoc();
                    beginLoc.isMacroID() && Lexer::getImmediateMacroName(beginLoc, sm(), lo()) == "Q_UNUSED") {
                    emitWarning(visitor.qunusedParamUsage, "Q_UNUSED used even though variable has usages");
                } else {
                    emitWarning(visitor.qunusedParamUsage, "void cast used even though variable has usages");
                }
            }
        }
    }
}
