/*
    SPDX-FileCopyrightText: 2015 Klarälvdalens Datakonsult AB a KDAB Group company info@kdab.com
    SPDX-FileContributor: Sérgio Martins <sergio.martins@kdab.com>

    SPDX-FileCopyrightText: 2015 Sergio Martins <smartins@kde.org>
    SPDX-FileCopyrightText: 2024 Alexander Lohnau <alexander.lohnau@gmx.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "qdatetime-utc.h"
#include "FixItUtils.h"
#include "Utils.h"

#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/Stmt.h>
#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/LLVM.h>
#include <llvm/Support/Casting.h>

#include <vector>

using namespace clang;

QDateTimeUtc::QDateTimeUtc(const std::string &name)
    : CheckBase(name)
{
}

void QDateTimeUtc::VisitStmt(clang::Stmt *stmt)
{
    auto *secondCall = dyn_cast<CXXMemberCallExpr>(stmt);
    if (!secondCall || !secondCall->getMethodDecl()) {
        return;
    }
    CXXMethodDecl *secondMethod = secondCall->getMethodDecl();
    const std::string secondMethodName = secondMethod->getQualifiedNameAsString();
    const bool isMSecSinceEpoc = secondMethodName == "QDateTime::toMSecsSinceEpoch";
    const bool isSecSinceEpoc = secondMethodName == "QDateTime::toSecsSinceEpoch" || secondMethodName == "QDateTime::toTime_t";
    const bool isToUtcConversion = secondMethodName == "QDateTime::toUTC";
    if (!isMSecSinceEpoc && !isSecSinceEpoc && !isToUtcConversion) {
        return;
    }

    std::vector<CallExpr *> chainedCalls = Utils::callListForChain(secondCall);
    if (chainedCalls.size() != 2) {
        return;
    }

    CallExpr *firstCall = chainedCalls[chainedCalls.size() - 1];
    FunctionDecl *firstFunc = firstCall->getDirectCallee();
    if (!firstFunc) {
        return;
    }

    if (auto *firstMethod = dyn_cast<CXXMethodDecl>(firstFunc); !firstMethod
        || (firstMethod->getQualifiedNameAsString() != "QDateTime::currentDateTime"
            && firstMethod->getQualifiedNameAsString() != "QDateTime::currentDateTimeUtc")) {
        return;
    }

    std::string replacement = "::currentDateTimeUtc()";
    if (isMSecSinceEpoc) {
        replacement = "::currentMSecsSinceEpoch()";
    } else if (isSecSinceEpoc) {
        replacement = "::currentSecsSinceEpoch()";
    }

    std::vector<FixItHint> fixits;
    clazy::transformTwoCallsIntoOneV2(astContext(), secondCall, replacement, fixits);
    emitWarning(stmt->getBeginLoc(), "Use QDateTime" + replacement + " instead. It is significantly faster", fixits);
}
