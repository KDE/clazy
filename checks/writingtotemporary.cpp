/*
   This file is part of the clang-lazy static checker.

  Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
  Author: Sérgio Martins <sergio.martins@kdab.com>

  Copyright (C) 2015 Sergio Martins <smartins@kde.org>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

  As a special exception, permission is given to link this program
  with any edition of Qt, and distribute the resulting executable,
  without including the source code for Qt in the source distribution.
*/

#include "writingtotemporary.h"
#include "Utils.h"
#include "checkmanager.h"
#include "StringUtils.h"

#include <clang/AST/AST.h>
#include <clang/Lex/Lexer.h>

using namespace clang;
using namespace std;


WritingToTemporary::WritingToTemporary(const std::string &name)
    : CheckBase(name)
{
}

std::vector<string> WritingToTemporary::filesToIgnore() const
{
    static const vector<string> files = { "qstring.h" };
    return files;
}

static bool isAllowedClass(const string &className)
{
    static const vector<string> disallowed = { "QTextCursor", "QDomElement", "KConfigGroup", "QWebElement", "QScriptValue", "QTextLine", "QTextBlock" };
    return find(disallowed.cbegin(), disallowed.cend(), className) == disallowed.cend();
}

void WritingToTemporary::VisitStmt(clang::Stmt *stmt)
{
    CallExpr *callExpr = dyn_cast<CallExpr>(stmt);
    if (!callExpr)
        return;

    // For a chain like getFoo().setBar(), returns {setBar(), getFoo()}
    vector<CallExpr *> callExprs = Utils::callListForChain(callExpr);
    if (callExprs.size() < 2)
        return;

    CallExpr *firstCallToBeEvaluated = callExprs.at(callExprs.size() - 1); // This is the call to getFoo()
    FunctionDecl *firstFunc = firstCallToBeEvaluated->getDirectCallee();
    if (!firstFunc)
        return;

    CallExpr *secondCallToBeEvaluated = callExprs.at(callExprs.size() - 2); // This is the call to setBar()
    FunctionDecl *secondFunc = secondCallToBeEvaluated->getDirectCallee();
    if (!secondFunc)
        return;

    CXXMethodDecl *secondMethod = dyn_cast<CXXMethodDecl>(secondFunc);
    if (!secondMethod || secondMethod->isConst() || secondMethod->isStatic())
        return;

    CXXRecordDecl *record = secondMethod->getParent();
    if (!record)
        return;

    if (!isAllowedClass(record->getNameAsString()))
        return;

    QualType qt = firstFunc->getReturnType();
    const Type *firstFuncReturnType = qt.getTypePtrOrNull();
    if (!firstFuncReturnType || firstFuncReturnType->isPointerType() || firstFuncReturnType->isReferenceType())
        return;

    qt = secondFunc->getReturnType();
    const Type *secondFuncReturnType = qt.getTypePtrOrNull();
    if (!secondFuncReturnType || !secondFuncReturnType->isVoidType())
        return;

    if (!stringStartsWith(secondFunc->getNameAsString(), "set"))
        return;

    emitWarning(stmt->getLocStart(), "Call to temporary is a no-op: " + secondFunc->getQualifiedNameAsString());
}

REGISTER_CHECK("writing-to-temporary", WritingToTemporary)
