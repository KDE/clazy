/*
   This file is part of the clazy static checker.

  Copyright (C) 2016 Sergio Martins <smartins@kde.org>

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

#include "lambda-in-connect.h"
#include "Utils.h"
#include "checkmanager.h"
#include "StringUtils.h"
#include "HierarchyUtils.h"
#include "ContextUtils.h"
#include "QtUtils.h"
#include <clang/AST/AST.h>
#include <clang/Lex/Lexer.h>

using namespace clang;
using namespace std;


LambdaInConnect::LambdaInConnect(const std::string &name, ClazyContext *context)
    : CheckBase(name, context)
{
}

void LambdaInConnect::VisitStmt(clang::Stmt *stmt)
{
    auto lambda = dyn_cast<LambdaExpr>(stmt);
    if (!lambda)
        return;

    auto callExpr = HierarchyUtils::getFirstParentOfType<CallExpr>(m_context->parentMap, lambda);
    if (StringUtils::qualifiedMethodName(callExpr) != "QObject::connect")
        return;

    ValueDecl *senderDecl = QtUtils::signalSenderForConnect(callExpr);
    if (senderDecl) {
        const Type *t = senderDecl->getType().getTypePtrOrNull();
        if (t && !t->isPointerType())
            return;
    }

    for (auto capture : lambda->captures()) {
        if (capture.getCaptureKind() == clang::LCK_ByRef && ContextUtils::isValueDeclInFunctionContext(capture.getCapturedVar()))
            emitWarning(capture.getLocation(), "capturing local variable by reference in lambda");
    }
}


REGISTER_CHECK("lambda-in-connect", LambdaInConnect, CheckLevel0)
