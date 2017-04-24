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

#include "writingtotemporary.h"
#include "Utils.h"
#include "checkmanager.h"
#include "StringUtils.h"

#include <clang/AST/AST.h>
#include <clang/Lex/Lexer.h>

using namespace clang;
using namespace std;


WritingToTemporary::WritingToTemporary(const std::string &name, ClazyContext *context)
    : CheckBase(name, context)
    , m_widenCriteria(isOptionSet("widen-criteria"))
{
    m_filesToIgnore = { "qstring.h" };
}

vector<string> WritingToTemporary::supportedOptions() const
{
    static const vector<string> options = { "widen-criteria" };
    return options;
}

static bool isDisallowedClass(const string &className)
{
    static const vector<string> disallowed = { "QTextCursor", "QDomElement", "KConfigGroup", "QWebElement",
                                               "QScriptValue", "QTextLine", "QTextBlock", "QDomNode",
                                               "QJSValue", "QTextTableCell" };
    return clazy_std::contains(disallowed, className);
}

static bool isDisallowedMethod(const string &name)
{
    static const vector<string> disallowed = { "QColor::getCmyk", "QColor::getCmykF" };
    return clazy_std::contains(disallowed, name);
}

static bool isKnownType(const string &className)
{
    static const vector<string> types = { "QList", "QVector", "QMap", "QHash", "QString", "QSet",
                                          "QByteArray", "QUrl", "QVarLengthArray", "QLinkedList",
                                          "QRect", "QRectF", "QBitmap", "QVector2D", "QVector3D",
                                          "QVector4D", "QSize", "QSizeF", "QSizePolicy", "QPoint",
                                          "QPointF", "QColor" };

    return clazy_std::contains(types, className);
}

void WritingToTemporary::VisitStmt(clang::Stmt *stmt)
{
    CallExpr *callExpr = dyn_cast<CallExpr>(stmt);
    if (!callExpr)
        return;

    if (shouldIgnoreFile(stmt->getLocStart()))
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
    if (!record || isDisallowedClass(record->getNameAsString()))
        return;

    QualType qt = firstFunc->getReturnType();
    const Type *firstFuncReturnType = qt.getTypePtrOrNull();
    if (!firstFuncReturnType || firstFuncReturnType->isPointerType() || firstFuncReturnType->isReferenceType())
        return;

    qt = secondFunc->getReturnType();
    const Type *secondFuncReturnType = qt.getTypePtrOrNull();
    if (!secondFuncReturnType || !secondFuncReturnType->isVoidType())
        return;

    if (!m_widenCriteria && !isKnownType(record->getNameAsString()) && !clazy_std::startsWith(secondFunc->getNameAsString(), "set"))
        return;

    const string &methodName = secondMethod->getQualifiedNameAsString();
    if (isDisallowedMethod(methodName))
        return;

    emitWarning(stmt->getLocStart(), "Call to temporary is a no-op: " + methodName);
}

REGISTER_CHECK_WITH_FLAGS("writing-to-temporary", WritingToTemporary, CheckLevel0)
