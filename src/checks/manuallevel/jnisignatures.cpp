/*
    SPDX-FileCopyrightText: 2020 Klarälvdalens Datakonsult AB a KDAB Group company info@kdab.com
    SPDX-FileContributor: Nicolas Fella <nicolas.fella@kdab.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "jnisignatures.h"
#include "HierarchyUtils.h"
#include "StringUtils.h"
#include "clazy_stl.h"

#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/OperationKinds.h>
#include <clang/AST/Stmt.h>
#include <clang/Basic/LLVM.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/SourceManager.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Casting.h>

class ClazyContext;

using namespace clang;

static const std::regex methodSignatureRegex("\\((\\[?([ZBCSIJFD]|L([a-zA-Z]+\\/)*[a-zA-Z]+;))*\\)\\[?([ZBCSIJFD]|L([a-zA-Z]+\\/)*[a-zA-Z]+;|V)");

static const std::regex classNameRegex("([a-zA-Z]+\\/)*[a-zA-Z]+");

static const std::regex methodNameRegex("[a-zA-Z]+");

JniSignatures::JniSignatures(const std::string &name, ClazyContext *context)
    : CheckBase(name, context, Option_CanIgnoreIncludes)
{
}

bool checkSignature(const std::string &signature, const std::regex &expr)
{
    std::smatch match;
    return std::regex_match(signature, match, expr);
}

template<typename T>
void JniSignatures::checkArgAt(T *call, unsigned int index, const std::regex &expr, const std::string &errorMessage)
{
    if (call->getNumArgs() < index + 1) {
        return;
    }

    auto *stringLiteral = clazy::getFirstChildOfType2<StringLiteral>(call->getArg(index));

    if (!stringLiteral) {
        return;
    }

    if (stringLiteral->getCharByteWidth() != 1) {
        return;
    }

    const std::string signature = stringLiteral->getString().str();

    const bool valid = checkSignature(signature, expr);

    if (!valid) {
        emitWarning(call, errorMessage + ": '" + signature + "'");
    }
}

void JniSignatures::checkFunctionCall(Stmt *stm)
{
    auto *callExpr = dyn_cast<CallExpr>(stm);
    if (!callExpr) {
        return;
    }
    auto *funDecl = callExpr->getDirectCallee();
    if (!funDecl) {
        return;
    }

    const std::string qualifiedName = funDecl->getQualifiedNameAsString();
    if (!clazy::startsWith(qualifiedName, "QAndroidJniObject::")) {
        return;
    }

    const std::string name = static_cast<std::string>(clazy::name(funDecl));

    if (name == "callObjectMethod" || name == "callMethod") {
        checkArgAt(callExpr, 0, methodNameRegex, "Invalid method name");
        checkArgAt(callExpr, 1, methodSignatureRegex, "Invalid method signature");
    } else if (name == "callStaticObjectMethod" || name == "callStaticMethod") {
        checkArgAt(callExpr, 0, classNameRegex, "Invalid class name");
        checkArgAt(callExpr, 1, methodNameRegex, "Invalid method name");
        checkArgAt(callExpr, 2, methodSignatureRegex, "Invalid method signature");
    }
}

void JniSignatures::checkConstructorCall(Stmt *stm)
{
    auto *constructExpr = dyn_cast<CXXConstructExpr>(stm);
    if (!constructExpr) {
        return;
    }
    auto *funDecl = constructExpr->getConstructor();

    const std::string qualifiedName = funDecl->getQualifiedNameAsString();
    if (qualifiedName != "QAndroidJniObject::QAndroidJniObject") {
        return;
    }

    checkArgAt(constructExpr, 0, classNameRegex, "Invalid class name");
    checkArgAt(constructExpr, 1, methodSignatureRegex, "Invalid constructor signature");
}

void JniSignatures::VisitStmt(Stmt *stm)
{
    checkConstructorCall(stm);
    checkFunctionCall(stm);
}
