/*
    SPDX-FileCopyrightText: 2015 Klarälvdalens Datakonsult AB a KDAB Group company info@kdab.com
    SPDX-FileContributor: Sérgio Martins <sergio.martins@kdab.com>

    SPDX-FileCopyrightText: 2015 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "writing-to-temporary.h"
#include "Utils.h"
#include "clazy_stl.h"

#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/Expr.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/Type.h>
#include <clang/Basic/LLVM.h>
#include <llvm/Support/Casting.h>

#include <vector>

using namespace clang;

WritingToTemporary::WritingToTemporary(const std::string &name, ClazyContext *context)
    : CheckBase(name, context, Option_CanIgnoreIncludes)
    , m_widenCriteria(isOptionSet("widen-criteria"))
{
    m_filesToIgnore = {"qstring.h"};
}

static bool isDisallowedClass(const std::string &className)
{
    static const std::vector<std::string> disallowed =
        {"QTextCursor", "QDomElement", "KConfigGroup", "QWebElement", "QScriptValue", "QTextLine", "QTextBlock", "QDomNode", "QJSValue", "QTextTableCell"};
    return clazy::contains(disallowed, className);
}

static bool isDisallowedMethod(const std::string &name)
{
    static const std::vector<std::string> disallowed = {"QColor::getCmyk", "QColor::getCmykF"};
    return clazy::contains(disallowed, name);
}

static bool isKnownType(const std::string &className)
{
    static const std::vector<std::string> types = {"QList",           "QVector",     "QMap",        "QHash",  "QString", "QSet",      "QByteArray", "QUrl",
                                                   "QVarLengthArray", "QLinkedList", "QRect",       "QRectF", "QBitmap", "QVector2D", "QVector3D",  "QVector4D",
                                                   "QSize",           "QSizeF",      "QSizePolicy", "QPoint", "QPointF", "QColor"};

    return clazy::contains(types, className);
}

void WritingToTemporary::VisitStmt(clang::Stmt *stmt)
{
    auto *callExpr = dyn_cast<CallExpr>(stmt);
    if (!callExpr) {
        return;
    }

    if (shouldIgnoreFile(stmt->getBeginLoc())) {
        return;
    }

    // For a chain like getFoo().setBar(), returns {setBar(), getFoo()}
    std::vector<CallExpr *> callExprs = Utils::callListForChain(callExpr); // callExpr is the call to setBar()
    if (callExprs.size() < 2) {
        return;
    }

    CallExpr *firstCallToBeEvaluated = callExprs.at(callExprs.size() - 1); // This is the call to getFoo()
    FunctionDecl *firstFunc = firstCallToBeEvaluated->getDirectCallee();
    if (!firstFunc) {
        return;
    }

    CallExpr *secondCallToBeEvaluated = callExprs.at(callExprs.size() - 2); // This is the call to setBar()
    FunctionDecl *secondFunc = secondCallToBeEvaluated->getDirectCallee();
    if (!secondFunc) {
        return;
    }

    auto *secondMethod = dyn_cast<CXXMethodDecl>(secondFunc);
    if (!secondMethod || secondMethod->isConst() || secondMethod->isStatic()) {
        return;
    }

    CXXRecordDecl *record = secondMethod->getParent();
    if (!record || isDisallowedClass(record->getNameAsString())) {
        return;
    }

    QualType qt = firstFunc->getReturnType();
    const Type *firstFuncReturnType = qt.getTypePtrOrNull();
    if (!firstFuncReturnType || firstFuncReturnType->isPointerType() || firstFuncReturnType->isReferenceType()) {
        return;
    }

    qt = secondFunc->getReturnType();
    const Type *secondFuncReturnType = qt.getTypePtrOrNull();
    if (!secondFuncReturnType || !secondFuncReturnType->isVoidType()) {
        return;
    }

    if (!m_widenCriteria && !isKnownType(record->getNameAsString()) && !clazy::startsWith(secondFunc->getNameAsString(), "set")) {
        return;
    }

    const std::string &methodName = secondMethod->getQualifiedNameAsString();
    if (isDisallowedMethod(methodName)) {
        return;
    }

    emitWarning(stmt->getBeginLoc(), "Call to temporary is a no-op: " + methodName);
}
