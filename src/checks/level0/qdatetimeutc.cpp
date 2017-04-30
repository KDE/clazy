/*
   This file is part of the clazy static checker.

  Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
  Author: Sérgio Martins <sergio.martins@kdab.com>

  Copyright (C) 2015 Sergio Martins <smartins@kde.org>

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

#include "qdatetimeutc.h"
#include "Utils.h"
#include "checkmanager.h"
#include "StringUtils.h"
#include "FixItUtils.h"

#include <clang/AST/AST.h>
#include <clang/Lex/Lexer.h>

using namespace clang;
using namespace std;

enum Fixit {
    FixitNone = 0,
    FixitAll = 0x1 // More granularity isn't needed I guess
};

QDateTimeUtc::QDateTimeUtc(const std::string &name, ClazyContext *context)
    : CheckBase(name, context)
{
}

void QDateTimeUtc::VisitStmt(clang::Stmt *stmt)
{
    CXXMemberCallExpr *secondCall = dyn_cast<CXXMemberCallExpr>(stmt);
    if (!secondCall || !secondCall->getMethodDecl())
        return;
    CXXMethodDecl *secondMethod = secondCall->getMethodDecl();
    const string secondMethodName = secondMethod->getQualifiedNameAsString();
    const bool isTimeT = secondMethodName == "QDateTime::toTime_t";
    if (!isTimeT && secondMethodName != "QDateTime::toUTC")
        return;

    vector<CallExpr*> chainedCalls = Utils::callListForChain(secondCall);
    if (chainedCalls.size() < 2)
        return;

    CallExpr *firstCall = chainedCalls[chainedCalls.size() - 1];
    FunctionDecl *firstFunc = firstCall->getDirectCallee();
    if (!firstFunc)
        return;

    CXXMethodDecl *firstMethod = dyn_cast<CXXMethodDecl>(firstFunc);
    if (!firstMethod || firstMethod->getQualifiedNameAsString() != "QDateTime::currentDateTime")
        return;

    std::string replacement = "::currentDateTimeUtc()";
    if (isTimeT) {
        replacement += ".toTime_t()";
    }

    std::vector<FixItHint> fixits;
    if (isFixitEnabled(FixitAll)) {
        const bool success = FixItUtils::transformTwoCallsIntoOneV2(&m_astContext, secondCall, replacement, fixits);
        if (!success) {
            queueManualFixitWarning(secondCall->getLocStart(), FixitAll);
        }
    }

    emitWarning(stmt->getLocStart(), "Use QDateTime" + replacement + " instead", fixits);
}

const char *const s_checkName = "qdatetime-utc";
REGISTER_CHECK(s_checkName, QDateTimeUtc, CheckLevel0)
REGISTER_FIXIT(FixitAll, "fix-qdatetime-utc", s_checkName)
