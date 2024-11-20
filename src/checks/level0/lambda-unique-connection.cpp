/*
    SPDX-FileCopyrightText: 2017 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "lambda-unique-connection.h"
#include "HierarchyUtils.h"
#include "QtUtils.h"

#include <clang/AST/Decl.h>
#include <clang/AST/DeclTemplate.h>
#include <clang/AST/Expr.h>
#include <clang/AST/Stmt.h>
#include <clang/Basic/LLVM.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Casting.h>

#include <vector>

class ClazyContext;
namespace clang
{
class CXXMethodDecl;
} // namespace clang

using namespace clang;

LambdaUniqueConnection::LambdaUniqueConnection(const std::string &name, ClazyContext *context)
    : CheckBase(name, context, Option_CanIgnoreIncludes)
{
}

void LambdaUniqueConnection::VisitStmt(clang::Stmt *stmt)
{
    auto *call = dyn_cast<CallExpr>(stmt);
    if (!call) {
        return;
    }

    // We want this signature:
    // connect(const QObject *sender, PointerToMemberFunction signal, const QObject *context, Functor functor, Qt::ConnectionType type)

    FunctionDecl *func = call->getDirectCallee();
    if (!func || func->getNumParams() != 5 || !func->isTemplateInstantiation() || !clazy::isConnect(func) || !clazy::connectHasPMFStyle(func)) {
        return;
    }

    Expr *typeArg = call->getArg(4); // The type

    std::vector<DeclRefExpr *> result;
    clazy::getChilds(typeArg, result);

    bool found = false;
    for (auto *declRef : result) {
        if (auto *enumConstant = dyn_cast<EnumConstantDecl>(declRef->getDecl())) {
            if (clazy::name(enumConstant) == "UniqueConnection") {
                found = true;
                break;
            }
        }
    }

    if (!found) {
        return;
    }

    FunctionTemplateSpecializationInfo *tsi = func->getTemplateSpecializationInfo();
    if (!tsi) {
        return;
    }
    FunctionTemplateDecl *temp = tsi->getTemplate();
    const TemplateParameterList *tempParams = temp->getTemplateParameters();
    if (tempParams->size() != 2) {
        return;
    }

    const CXXMethodDecl *method = clazy::pmfFromConnect(call, 3);
    if (method) {
        // How else to detect if it's the right overload ? It's all templated stuff with the same
        // names for all the template arguments
        return;
    }

    emitWarning(typeArg, "UniqueConnection is not supported with non-member functions");
}
