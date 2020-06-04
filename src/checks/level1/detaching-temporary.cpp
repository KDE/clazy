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


#include "detaching-temporary.h"
#include "Utils.h"
#include "StringUtils.h"
#include "QtUtils.h"
#include "SourceCompatibilityHelpers.h"
#include "checkbase.h"
#include "clazy_stl.h"

#include <clang/AST/DeclCXX.h>
#include <clang/AST/Expr.h>
#include <clang/AST/Decl.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/Type.h>
#include <clang/Basic/LLVM.h>
#include <llvm/Support/Casting.h>

#include <unordered_map>
#include <utility>

class ClazyContext;

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
    return clazy::contains(allowed, className);
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
    return clazy::contains(allowed, methodName);
}

void DetachingTemporary::VisitStmt(clang::Stmt *stm)
{
    auto callExpr = dyn_cast<CallExpr>(stm);
    if (!callExpr)
        return;


    // For a chain like getList().first(), returns {first(), getList()}
    vector<CallExpr *> callExprs = Utils::callListForChain(callExpr); // callExpr would be first()
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

    auto firstMethod = dyn_cast<CXXMethodDecl>(firstFunc);
    if (isAllowedChainedMethod(clazy::qualifiedMethodName(firstFunc))) {
        return;
    }

    if (firstMethod && isAllowedChainedClass(firstMethod->getParent()->getNameAsString())) {
        return;
    }

    // Check if this is a QGlobalStatic
    if (firstMethod && clazy::name(firstMethod->getParent()) == "QGlobalStatic") {
        return;
    }

    CallExpr *secondCallToBeEvaluated = callExprs.at(callExprs.size() - 2); // This is the call to first()
    FunctionDecl *detachingFunc = secondCallToBeEvaluated->getDirectCallee();
    auto detachingMethod = detachingFunc ? dyn_cast<CXXMethodDecl>(detachingFunc) : nullptr;
    const Type *detachingMethodReturnType = detachingMethod ? detachingMethod->getReturnType().getTypePtrOrNull() : nullptr;
    if (!detachingMethod || !detachingMethodReturnType)
        return;

    // Check if it's one of the implicit shared classes
    CXXRecordDecl *classDecl = detachingMethod->getParent();
    StringRef className = clazy::name(classDecl);

    const std::unordered_map<string, std::vector<StringRef>> &methodsByType = clazy::detachingMethods();
    auto it = methodsByType.find(static_cast<std::string>(className));
    auto it2 = m_writeMethodsByType.find(className);

    std::vector<StringRef> allowedFunctions;
    std::vector<StringRef> allowedWriteFunctions;
    if (it != methodsByType.end()) {
        allowedFunctions = it->second;
    }

    if (it2 != m_writeMethodsByType.end()) {
        allowedWriteFunctions = it2->second;
    }

    // Check if it's one of the detaching methods
    StringRef functionName = clazy::name(detachingMethod);

    string error;

    const bool isReadFunction = clazy::contains(allowedFunctions, functionName);
    const bool isWriteFunction = clazy::contains(allowedWriteFunctions, functionName);

    if (isReadFunction || isWriteFunction) {
        bool returnTypeIsIterator = false;
        CXXRecordDecl *returnRecord = detachingMethodReturnType->getAsCXXRecordDecl();
        if (returnRecord)
            returnTypeIsIterator = clazy::name(returnRecord) == "iterator";

        if (isWriteFunction && (detachingMethodReturnType->isVoidType() || returnTypeIsIterator)) {
            error = std::string("Modifying temporary container is pointless and it also detaches");
        } else {
            error = std::string("Don't call ") + clazy::qualifiedMethodName(detachingMethod) + std::string("() on temporary");
        }
    }


    if (!error.empty())
        emitWarning(clazy::getLocStart(stm), error.c_str());
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

    StringRef className = clazy::name(record);

    auto it = m_writeMethodsByType.find(className);
    if (it != m_writeMethodsByType.cend()) {
        const auto &methods = it->second;
        if (clazy::contains(methods, clazy::name(method)))
            return true;
    }

    return false;
}
