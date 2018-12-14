/*
    This file is part of the clazy static checker.

    Copyright (C) 2016-2017 Sergio Martins <smartins@kde.org>

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
#include "ClazyContext.h"
#include "StringUtils.h"
#include "HierarchyUtils.h"
#include "ContextUtils.h"
#include "QtUtils.h"

#include <clang/AST/Decl.h>
#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/LambdaCapture.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/Type.h>
#include <clang/Basic/LLVM.h>
#include <clang/Basic/Lambda.h>
#include <llvm/ADT/iterator_range.h>
#include <llvm/Support/Casting.h>

using namespace clang;
using namespace std;


LambdaInConnect::LambdaInConnect(const std::string &name, ClazyContext *context)
    : CheckBase(name, context, Option_CanIgnoreIncludes)
{
}

void LambdaInConnect::VisitStmt(clang::Stmt *stmt)
{
    auto lambda = dyn_cast<LambdaExpr>(stmt);
    if (!lambda)
        return;

    auto captures = lambda->captures();
    if (captures.begin() == captures.end())
        return;

    auto callExpr = clazy::getFirstParentOfType<CallExpr>(m_context->parentMap, lambda);
    if (clazy::qualifiedMethodName(callExpr) != "QObject::connect")
        return;

    ValueDecl *senderDecl = clazy::signalSenderForConnect(callExpr);
    if (senderDecl) {
        const Type *t = senderDecl->getType().getTypePtrOrNull();
        if (t && !t->isPointerType())
            return;
    }

    ValueDecl *receiverDecl = clazy::signalReceiverForConnect(callExpr);

    for (auto capture : captures) {
        if (capture.getCaptureKind() == clang::LCK_ByRef) {
            VarDecl *declForCapture = capture.getCapturedVar();
            if (declForCapture && declForCapture != receiverDecl && clazy::isValueDeclInFunctionContext(declForCapture))
                emitWarning(capture.getLocation(), "captured local variable by reference might go out of scope before lambda is called");
        }
    }
}
