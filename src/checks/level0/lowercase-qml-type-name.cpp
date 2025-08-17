/*
    SPDX-FileCopyrightText: 2018 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "lowercase-qml-type-name.h"
#include "HierarchyUtils.h"
#include "QtUtils.h"
#include "TypeUtils.h"
#include "Utils.h"

#include <cctype>
#include <clang/AST/AST.h>

using namespace clang;

void LowercaseQMlTypeName::VisitStmt(clang::Stmt *stmt)
{
    auto *callExpr = dyn_cast<CallExpr>(stmt);
    if (!callExpr) {
        return;
    }

    const FunctionDecl *func = callExpr->getDirectCallee();
    if (!func) {
        return;
    }

    StringRef name = clazy::name(func);

    Expr *arg = nullptr;

    if (name == "qmlRegisterType" || name == "qmlRegisterUncreatableType") {
        arg = callExpr->getNumArgs() <= 3 ? nullptr : callExpr->getArg(3);
    }

    if (!arg) {
        return;
    }

    auto *literal = clazy::getFirstChildOfType2<StringLiteral>(arg);
    if (!literal) {
        return;
    }

    StringRef str = literal->getString();

    if (str.empty() || !isupper(str[0])) {
        emitWarning(arg, "QML types must begin with uppercase");
    }
}
