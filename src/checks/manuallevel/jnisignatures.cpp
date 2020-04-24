/*
    This file is part of the clazy static checker.

    Copyright (C) 2020 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
    Author: Nicolas Fella <nicolas.fella@kdab.com>

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

#include "jnisignatures.h"
#include "MacroUtils.h"
#include "FunctionUtils.h"
#include "StringUtils.h"
#include "SourceCompatibilityHelpers.h"
#include "clazy_stl.h"

#include <clang/AST/Expr.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/OperationKinds.h>
#include <clang/Basic/LLVM.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/SourceManager.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Casting.h>

#include <vector>

#include <iostream>

class ClazyContext;

using namespace clang;
using namespace std;

JniSignatures::JniSignatures(const std::string &name, ClazyContext *context)
    : CheckBase(name, context, Option_CanIgnoreIncludes)
{
}

bool checkSignature(std::string signature)
{
    static regex rx("\\((\\[?([ZBCSIJFD]|L([a-zA-Z]+\\/)*[a-zA-Z]+;))*\\)\\[?([ZBCSIJFD]|L([a-zA-Z]+\\/)*[a-zA-Z]+;|V)");

    smatch match;
    return regex_match(signature, match, rx);
}

template<typename T>
void JniSignatures::checkArgAt(T *call, unsigned int index)
{
    if (call->getNumArgs() < index + 1)
        return;

    StringLiteral *stringLiteral = clazy::getFirstChildOfType2<StringLiteral>(call->getArg(index));

    if (!stringLiteral)
        return;

    if (stringLiteral->getCharByteWidth() != 1)
        return;

    const std::string signature = stringLiteral->getString().str();

    const bool valid = checkSignature(signature);

    if (!valid) {
        emitWarning(call, "Invalid method signature: '" + signature + "'");
    }
}

bool JniSignatures::functionShouldBeChecked(FunctionDecl *funDecl)
{
    const std::string qualifiedName = funDecl->getQualifiedNameAsString();
    if (!clazy::startsWith(qualifiedName, "QAndroidJniObject::")) {
        return false;
    }

    const std::string name = clazy::name(funDecl);
    return name == "callObjectMethod"
        || name == "callMethod"
        || name == "callStaticObjectMethod"
        || name == "callStaticMethod";
}

void JniSignatures::checkFunctionCall(Stmt *stm)
{
    auto callExpr = dyn_cast<CallExpr>(stm);
    if (!callExpr)
        return;
    auto funDecl = callExpr->getDirectCallee();
    if (!funDecl) {
        return;
    }

    if (!functionShouldBeChecked(funDecl)) {
        return;
    }

    checkArgAt(callExpr, 1);
}

void JniSignatures::checkConstructorCall(Stmt *stm)
{
    auto constructExpr = dyn_cast<CXXConstructExpr>(stm);
    if (!constructExpr) {
        return;
    }
    auto funDecl = constructExpr->getConstructor();

    const std::string qualifiedName = funDecl->getQualifiedNameAsString();
    if (qualifiedName != "QAndroidJniObject::QAndroidJniObject") {
        return;
    }

    checkArgAt(constructExpr, 1);
}

void JniSignatures::VisitStmt(Stmt *stm)
{
    checkConstructorCall(stm);
    checkFunctionCall(stm);
}
