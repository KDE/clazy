/*
    SPDX-FileCopyrightText: 2015 Klarälvdalens Datakonsult AB a KDAB Group company info@kdab.com
    SPDX-FileContributor: Sérgio Martins <sergio.martins@kdab.com>

    SPDX-FileCopyrightText: 2015 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "qgetenv.h"
#include "FixItUtils.h"
#include "SourceCompatibilityHelpers.h"
#include "StringUtils.h"
#include "Utils.h"

#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/Stmt.h>
#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/LLVM.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Casting.h>

#include <vector>

class ClazyContext;

using namespace clang;

QGetEnv::QGetEnv(const std::string &name, ClazyContext *context)
    : CheckBase(name, context, Option_CanIgnoreIncludes)
{
}

void QGetEnv::VisitStmt(clang::Stmt *stmt)
{
    // Lets check only in function calls. Otherwise there are too many false positives, it's common
    // to implicit cast to bool when checking pointers for validity, like if (ptr)

    auto *memberCall = dyn_cast<CXXMemberCallExpr>(stmt);
    if (!memberCall) {
        return;
    }

    CXXMethodDecl *method = memberCall->getMethodDecl();
    if (!method) {
        return;
    }

    if (CXXRecordDecl *record = method->getParent(); !record || clazy::name(record) != "QByteArray") {
        return;
    }

    std::vector<CallExpr *> calls = Utils::callListForChain(memberCall);
    if (calls.size() != 2) {
        return;
    }

    CallExpr *qgetEnvCall = calls.back();
    if (FunctionDecl *func = qgetEnvCall->getDirectCallee(); !func || clazy::name(func) != "qgetenv") {
        return;
    }

    StringRef methodname = clazy::name(method);
    std::string errorMsg;
    std::string replacement;
    bool shouldIncludeBaseParameter = false;
    if (methodname == "isEmpty") {
        errorMsg = "qgetenv().isEmpty() allocates.";
        replacement = "qEnvironmentVariableIsEmpty";
    } else if (methodname == "isNull") {
        errorMsg = "qgetenv().isNull() allocates.";
        replacement = "qEnvironmentVariableIsSet";
    } else if (methodname == "toInt") {
        errorMsg = "qgetenv().toInt() is slow.";
        replacement = "qEnvironmentVariableIntValue";
        for (unsigned int i = 0; i < memberCall->getNumArgs(); ++i) {
            if (!dyn_cast<CXXDefaultArgExpr>(memberCall->getArg(i))) {
                if (i == 0) {
                    shouldIncludeBaseParameter = true;
                }
                if (i > 0) {
                    return; // Second toInt arg (base) is not supported by qEnvironmentVariableIntValue
                }
            }
        }
    } else {
        return;
    }

    std::string getEnvArgStr = Lexer::getSourceText(CharSourceRange::getTokenRange(qgetEnvCall->getArg(0)->getSourceRange()), sm(), lo()).str();
    if (shouldIncludeBaseParameter) {
        getEnvArgStr += ", " + Lexer::getSourceText(CharSourceRange::getTokenRange(memberCall->getArg(0)->getSourceRange()), sm(), lo()).str();
    }

    errorMsg += " Use " + replacement + "() instead";
    emitWarning(clazy::getLocStart(memberCall), errorMsg, {FixItHint::CreateReplacement(stmt->getSourceRange(), replacement + "(" + getEnvArgStr + ")")});
}
