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

#include "checkmanager.h"
#include "detachingtemporary.h"
#include "Utils.h"
#include "StringUtils.h"
#include "QtUtils.h"

#include <clang/AST/DeclCXX.h>
#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>

using namespace clang;
using namespace std;

DetachingTemporary::DetachingTemporary(const std::string &name, ClazyContext *context)
    : DetachingBase(name, context, Option_CanIgnoreIncludes)
{
    // Extra stuff that isn't really related to detachments but doesn't make sense to call on temporaries
    m_writeMethodsByType["QString"] = {"push_back", "push_front", "clear", "chop"};
    m_writeMethodsByType["QList"] = {"takeAt", "takeFirst", "takeLast", "removeOne", "removeAll", "erase"};
    m_writeMethodsByType["QVector"] = { "fill", "insert"};
    m_writeMethodsByType["QMap"] = { "erase", "insert", "insertMulti", "remove", "take"};
    m_writeMethodsByType["QHash"] = { "erase", "insert", "insertMulti", "remove", "take"};
    m_writeMethodsByType["QMultiHash"] = m_writeMethodsByType["QHash"];
    m_writeMethodsByType["QMultiMap"] = m_writeMethodsByType["QMap"];
    m_writeMethodsByType["QLinkedList"] = {"takeFirst", "takeLast", "removeOne", "removeAll", "erase"};
    m_writeMethodsByType["QSet"] = {"erase", "insert"};
    m_writeMethodsByType["QStack"] = {"push", "swap"};
    m_writeMethodsByType["QQueue"] = {"enqueue", "swap"};
    m_writeMethodsByType["QListSpecialMethods"] = {"sort", "replaceInStrings", "removeDuplicates"};
    m_writeMethodsByType["QStringList"] = m_writeMethodsByType["QListSpecialMethods"];
}

bool isAllowedChainedClass(const std::string &className)
{
    static const vector<string> allowed = {"QString", "QByteArray", "QVariant"};
    return clazy_std::contains(allowed, className);
}

bool isAllowedChainedMethod(const std::string &methodName)
{
    static const vector<string> allowed = {"QMap::keys", "QMap::values", "QHash::keys", "QMap::values",
                                           "QApplication::topLevelWidgets", "QAbstractItemView::selectedIndexes",
                                           "QListWidget::selectedItems", "QFile::encodeName", "QFile::decodeName",
                                           "QItemSelectionModel::selectedRows", "QTreeWidget::selectedItems",
                                           "QTableWidget::selectedItems", "QNetworkReply::rawHeaderList",
                                           "Mailbox::address", "QItemSelection::indexes", "QItemSelectionModel::selectedIndexes",
                                           "QMimeData::formats", "i18n", "QAbstractTransition::targetStates"};
    return clazy_std::contains(allowed, methodName);
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

    if (firstFuncReturnType->isReferenceType() || firstFuncReturnType->isPointerType())
        return;

    if (qt.isConstQualified()) {
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
    CXXMethodDecl *detachingMethod = detachingFunc ? dyn_cast<CXXMethodDecl>(detachingFunc) : nullptr;
    const Type *detachingMethodReturnType = detachingMethod ? detachingMethod->getReturnType().getTypePtrOrNull() : nullptr;
    if (!detachingMethod || !detachingMethodReturnType)
        return;

    // Check if it's one of the implicit shared classes
    CXXRecordDecl *classDecl = detachingMethod->getParent();
    const std::string className = classDecl->getNameAsString();

    const std::unordered_map<string, std::vector<string> > &methodsByType = QtUtils::detachingMethods();
    auto it = methodsByType.find(className);
    auto it2 = m_writeMethodsByType.find(className);

    std::vector<std::string> allowedFunctions;
    std::vector<std::string> allowedWriteFunctions;
    if (it != methodsByType.end()) {
        allowedFunctions = it->second;
    }

    if (it2 != m_writeMethodsByType.end()) {
        allowedWriteFunctions = it2->second;
    }

    // Check if it's one of the detaching methods
    const std::string functionName = detachingMethod->getNameAsString();

    string error;

    const bool isReadFunction = clazy_std::contains(allowedFunctions, functionName);
    const bool isWriteFunction = clazy_std::contains(allowedWriteFunctions, functionName);

    if (isReadFunction || isWriteFunction) {
        bool returnTypeIsIterator = false;
        CXXRecordDecl *returnRecord = detachingMethodReturnType->getAsCXXRecordDecl();
        if (returnRecord) {
            returnTypeIsIterator = returnRecord->getNameAsString() == "iterator";
        }

        if (isWriteFunction && (detachingMethodReturnType->isVoidType() || returnTypeIsIterator)) {
            error = std::string("Modifying temporary container is pointless and it also detaches");
        } else {
            error = std::string("Don't call ") + StringUtils::qualifiedMethodName(detachingMethod) + std::string("() on temporary");
        }
    }


    if (!error.empty())
        emitWarning(stm->getLocStart(), error.c_str());
}

bool DetachingTemporary::isDetachingMethod(CXXMethodDecl *method) const
{
    if (!method)
        return false;

    CXXRecordDecl *record = method->getParent();
    if (!record)
        return false;

    if (DetachingBase::isDetachingMethod(method))
        return true;

    const string className = record->getNameAsString();

    auto it = m_writeMethodsByType.find(className);
    if (it != m_writeMethodsByType.cend()) {
        const auto &methods = it->second;
        if (clazy_std::contains(methods, method->getNameAsString()))
            return true;
    }

    return false;
}

REGISTER_CHECK("detaching-temporary", DetachingTemporary, CheckLevel1)
