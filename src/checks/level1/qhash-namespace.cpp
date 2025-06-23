/*
    SPDX-FileCopyrightText: 2017 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "qhash-namespace.h"
#include "ClazyContext.h"
#include "ContextUtils.h"
#include "PreProcessorVisitor.h"
#include "StringUtils.h"

#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/Basic/LLVM.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Casting.h>

using namespace clang;

QHashNamespace::QHashNamespace(const std::string &name)
    : CheckBase(name)
{
}

void QHashNamespace::VisitDecl(clang::Decl *decl)
{
    auto *func = dyn_cast<FunctionDecl>(decl);
    if (!func || isa<CXXMethodDecl>(func) || func->getNumParams() == 0 || clazy::name(func) != "qHash") {
        return;
    }

    ParmVarDecl *firstArg = func->getParamDecl(0);
    NamespaceDecl *argumentNS = clazy::namespaceForType(firstArg->getType());
    NamespaceDecl *qHashNS = clazy::namespaceForFunction(func);

    std::string msg;
    if (qHashNS && argumentNS) {
        const std::string argumentNSstr = argumentNS->getQualifiedNameAsString();
        const std::string qhashNSstr = qHashNS->getQualifiedNameAsString();
        if (argumentNSstr != qhashNSstr) {
            msg = "Move qHash(" + clazy::simpleTypeName(firstArg->getType(), lo()) + ") to " + argumentNSstr + " namespace for ADL lookup";
        }
    } else if (qHashNS && !argumentNS) {
        msg = "Move qHash(" + clazy::simpleTypeName(firstArg->getType(), lo()) + ") out of namespace " + qHashNS->getQualifiedNameAsString();
    } else if (!qHashNS && argumentNS) {
        msg =
            "Move qHash(" + clazy::simpleTypeName(firstArg->getType(), lo()) + ") into " + argumentNS->getQualifiedNameAsString() + " namespace for ADL lookup";
    }

    if (!msg.empty()) {
        emitWarning(decl, msg);
    }

    if (m_context->isQtDeveloper()) {
        PreProcessorVisitor *preProcessorVisitor = m_context->preprocessorVisitor;
        if (preProcessorVisitor && !preProcessorVisitor->isBetweenQtNamespaceMacros(func->getBeginLoc())) {
            emitWarning(decl, "qHash(" + clazy::simpleTypeName(firstArg->getType(), lo()) + ") must be declared before QT_END_NAMESPACE");
        }
    }
}
