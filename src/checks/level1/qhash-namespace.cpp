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

#include "qhash-namespace.h"
#include "ContextUtils.h"
#include "StringUtils.h"
#include "ClazyContext.h"
#include "PreProcessorVisitor.h"
#include "SourceCompatibilityHelpers.h"

#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/Basic/LLVM.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Casting.h>

namespace clang {
class Decl;
}  // namespace clang

using namespace clang;
using namespace std;


QHashNamespace::QHashNamespace(const std::string &name, ClazyContext *context)
    : CheckBase(name, context)
{
    if (context->isQtDeveloper())
        context->enablePreprocessorVisitor();
}

void QHashNamespace::VisitDecl(clang::Decl *decl)
{
    auto func = dyn_cast<FunctionDecl>(decl);
    if (!func || isa<CXXMethodDecl>(func) || func->getNumParams() == 0 || clazy::name(func) != "qHash")
        return;

    ParmVarDecl *firstArg = func->getParamDecl(0);
    NamespaceDecl *argumentNS = clazy::namespaceForType(firstArg->getType());
    NamespaceDecl *qHashNS =  clazy::namespaceForFunction(func);

    std::string msg;
    if (qHashNS && argumentNS) {
        const string argumentNSstr = argumentNS->getQualifiedNameAsString();
        const string qhashNSstr = qHashNS->getQualifiedNameAsString();
        if (argumentNSstr != qhashNSstr)
            msg = "Move qHash(" + clazy::simpleTypeName(firstArg->getType(), lo()) + ") to " + argumentNSstr + " namespace for ADL lookup";
    } else if (qHashNS && !argumentNS) {
        msg = "Move qHash(" + clazy::simpleTypeName(firstArg->getType(), lo()) + ") out of namespace " + qHashNS->getQualifiedNameAsString();
    } else if (!qHashNS && argumentNS) {
        msg = "Move qHash(" + clazy::simpleTypeName(firstArg->getType(), lo()) + ") into " + argumentNS->getQualifiedNameAsString() + " namespace for ADL lookup";
    }

    if (!msg.empty())
        emitWarning(decl, msg);

    if (m_context->isQtDeveloper()) {
        PreProcessorVisitor *preProcessorVisitor = m_context->preprocessorVisitor;
        if (preProcessorVisitor && !preProcessorVisitor->isBetweenQtNamespaceMacros(clazy::getLocStart(func))) {
            emitWarning(decl, "qHash(" + clazy::simpleTypeName(firstArg->getType(), lo()) + ") must be declared before QT_END_NAMESPACE");
        }
    }
}
