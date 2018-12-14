/*
  This file is part of the clazy static checker.

    Copyright (C) 2017 Sergio Martins <smartins@kde.org>

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
namespace clang {
class CXXMethodDecl;
}  // namespace clang

using namespace clang;
using namespace std;


LambdaUniqueConnection::LambdaUniqueConnection(const std::string &name, ClazyContext *context)
    : CheckBase(name, context, Option_CanIgnoreIncludes)
{
}

void LambdaUniqueConnection::VisitStmt(clang::Stmt *stmt)
{
    auto call = dyn_cast<CallExpr>(stmt);
    if (!call)
        return;

    // We want this signature:
    // connect(const QObject *sender, PointerToMemberFunction signal, const QObject *context, Functor functor, Qt::ConnectionType type)

    FunctionDecl *func = call->getDirectCallee();
    if (!func || func->getNumParams() != 5 || !func->isTemplateInstantiation() || !clazy::isConnect(func) || !clazy::connectHasPMFStyle(func))
        return;

    Expr *typeArg = call->getArg(4); // The type

    vector<DeclRefExpr*> result;
    clazy::getChilds(typeArg, result);

    bool found = false;
    for (auto declRef : result) {
        if (auto enumConstant = dyn_cast<EnumConstantDecl>(declRef->getDecl())) {
            if (clazy::name(enumConstant) == "UniqueConnection") {
                found = true;
                break;
            }
        }
    }

    if (!found)
        return;

    FunctionTemplateSpecializationInfo *tsi = func->getTemplateSpecializationInfo();
    if (!tsi)
        return;
    FunctionTemplateDecl *temp = tsi->getTemplate();
    const TemplateParameterList *tempParams = temp->getTemplateParameters();
    if (tempParams->size() != 2)
        return;

    CXXMethodDecl *method = clazy::pmfFromConnect(call, 3);
    if (method) {
        // How else to detect if it's the right overload ? It's all templated stuff with the same
        // names for all the template arguments
        return;
    }

    emitWarning(typeArg, "UniqueConnection is not supported with non-member functions");
}
