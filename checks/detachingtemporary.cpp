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

#include "checkmanager.h"
#include "detachingtemporary.h"
#include "Utils.h"
#include "StringUtils.h"

#include <clang/AST/DeclCXX.h>
#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>

using namespace clang;
using namespace std;

DetachingTemporary::DetachingTemporary(const std::string &name)
    : CheckBase(name)
{
    m_methodsByType["QList"] = {"first", "last", "begin", "end", "front", "back", "takeAt", "takeFirst", "takeLast", "removeOne", "removeAll", "erase"};
    m_methodsByType["QVector"] = {"first", "last", "begin", "end", "front", "back", "data", "fill", "insert" };
    m_methodsByType["QMap"] = {"begin", "end", "erase", "first", "find", "insert", "insertMulti", "last", "lowerBound", "remove", "take", "upperBound", "unite" };
    m_methodsByType["QHash"] = {"begin", "end", "erase", "find", "insert", "insertMulti", "remove", "take", "unite" };
    m_methodsByType["QLinkedList"] = {"first", "last", "begin", "end", "front", "back", "takeFirst", "takeLast", "removeOne", "removeAll", "erase"};
    m_methodsByType["QSet"] = {"begin", "end", "erase", "find", "insert", "intersect", "unite", "subtract"};
    m_methodsByType["QStack"] = {"push", "swap", "top"};
    m_methodsByType["QQueue"] = {"head", "enqueue", "swap"};
    m_methodsByType["QMultiMap"] = m_methodsByType["QMap"];
    m_methodsByType["QMultiHash"] = m_methodsByType["QHash"];
    m_methodsByType["QString"] = {"begin", "end", "data"};
    m_methodsByType["QByteArray"] = {"data"};
    m_methodsByType["QImage"] = {"bits", "scanLine"};
}

bool isAllowedChainedClass(const std::string &className)
{
    static const vector<string> allowed = {"QString", "QByteArray", "QVariant"};
    return find(allowed.cbegin(), allowed.cend(), className) != allowed.cend();
}

bool isAllowedChainedMethod(const std::string &methodName)
{
    static const vector<string> allowed = {"QMap::keys", "QMap::values", "QHash::keys", "QMap::values",
                                           "QApplication::topLevelWidgets", "QAbstractItemView::selectedIndexes",
                                           "QListWidget::selectedItems", "QFile::encodeName", "QFile::decodeName",
                                           "QItemSelectionModel::selectedRows", "QTreeWidget::selectedItems",
                                           "QTableWidget::selectedItems", "QNetworkReply::rawHeaderList",
                                           "Mailbox::address", "QItemSelection::indexes", "QItemSelectionModel::selectedIndexes"};
    return find(allowed.cbegin(), allowed.cend(), methodName) != allowed.cend();
}

void DetachingTemporary::VisitStmt(clang::Stmt *stm)
{
    CallExpr *callExpr = dyn_cast<CallExpr>(stm);
    if (!callExpr)
        return;


    // For a chain like getList().first(), returns {first(), getList()}
    vector<CallExpr *> callExprs = Utils::callListForChain(callExpr);
    if (callExprs.size() < 2)
        return;

    CallExpr *firstCallToBeEvaluated = callExprs.at(callExprs.size() - 1); // This is the call to getList()
    FunctionDecl *firstFunc = firstCallToBeEvaluated->getDirectCallee();
    if (!firstFunc)
        return;
    QualType qt = firstFunc->getReturnType();
    const Type *firstFuncReturnType = qt.getTypePtrOrNull();
    if (!firstFuncReturnType)
        return;

    if (firstFuncReturnType->isReferenceType())
        return;

    if (firstFuncReturnType->isPointerType()) {
        QualType qt = firstFuncReturnType->getPointeeType();
        if (qt.isConstQualified())
            return;
    } else if (qt.isConstQualified()) {
        return; // const doesn't detach
    }

    CXXMethodDecl *firstMethod = dyn_cast<CXXMethodDecl>(firstFunc);
    if (isAllowedChainedMethod(StringUtils::qualifiedMethodName(firstFunc))) {
        return;
    }

    if (firstMethod && isAllowedChainedClass(firstMethod->getParent()->getNameAsString())) {
        return;
    }

    // Check if this is a QGlobalStatic
    if (firstMethod && firstMethod->getParent()->getNameAsString() == "QGlobalStatic") {
        return;
    }

    CallExpr *secondCallToBeEvaluated = callExprs.at(callExprs.size() - 2); // This is the call to first()
    FunctionDecl *detachingFunc = secondCallToBeEvaluated->getDirectCallee();
    CXXMethodDecl *detachingMethod = dyn_cast<CXXMethodDecl>(detachingFunc);
    if (!detachingMethod)
        return;

    // Check if it's one of the implicit shared classes
    CXXRecordDecl *classDecl = detachingMethod->getParent();
    const std::string className = classDecl->getNameAsString();
    auto it = m_methodsByType.find(className);
    if (it == m_methodsByType.end())
        return;

    // Check if it's one of the detaching methods
    const std::string functionName = detachingMethod->getNameAsString();
    const auto &allowedFunctions = it->second;
    if (std::find(allowedFunctions.cbegin(), allowedFunctions.cend(), functionName) == allowedFunctions.cend())
        return;

    std::string error = std::string("Don't call ") + StringUtils::qualifiedMethodName(detachingMethod) + std::string("() on temporary");
    emitWarning(stm->getLocStart(), error.c_str());
}

REGISTER_CHECK("detaching-temporary", DetachingTemporary)
