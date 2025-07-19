/*
    SPDX-FileCopyrightText: 2015 Klarälvdalens Datakonsult AB a KDAB Group company info@kdab.com
    SPDX-FileContributor: Sérgio Martins <sergio.martins@kdab.com>

    SPDX-FileCopyrightText: 2015 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "detaching-temporary.h"
#include "QtUtils.h"
#include "StringUtils.h"
#include "Utils.h"
#include "checkbase.h"
#include "clazy_stl.h"

#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/Expr.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/Type.h>
#include <clang/Basic/LLVM.h>
#include <llvm/Support/Casting.h>

#include <unordered_map>
#include <utility>

using namespace clang;

DetachingTemporary::DetachingTemporary(const std::string &name)
    : DetachingBase(name, Option_CanIgnoreIncludes)
{
    // Extra stuff that isn't really related to detachments but doesn't make sense to call on temporaries
    m_writeMethodsByType["QString"] = {"push_back", "push_front", "clear", "chop"};
    m_writeMethodsByType["QList"] = {"takeAt", "takeFirst", "takeLast", "removeOne", "removeAll", "erase"};
    m_writeMethodsByType["QVector"] = {"fill", "insert"};
    m_writeMethodsByType["QMap"] = {"erase", "insert", "insertMulti", "remove", "take"};
    m_writeMethodsByType["QHash"] = {"erase", "insert", "insertMulti", "remove", "take"};
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
    static const std::vector<std::string> allowed = {"QString", "QByteArray", "QVariant"};
    return clazy::contains(allowed, className);
}

bool isAllowedChainedMethod(const std::string &methodName)
{
    static const std::vector<std::string> allowed = {
        "QMap::keys",
        "QMap::values",
        "QHash::keys",
        "QMap::values",
        "QApplication::topLevelWidgets",
        "QAbstractItemView::selectedIndexes",
        "QListWidget::selectedItems",
        "QFile::encodeName",
        "QFile::decodeName",
        "QItemSelectionModel::selectedRows",
        "QTreeWidget::selectedItems",
        "QTableWidget::selectedItems",
        "QNetworkReply::rawHeaderList",
        "Mailbox::address",
        "QItemSelection::indexes",
        "QItemSelectionModel::selectedIndexes",
        "QMimeData::formats",
        "i18n",
        "QAbstractTransition::targetStates",
    };
    return clazy::contains(allowed, methodName);
}

void DetachingTemporary::VisitStmt(clang::Stmt *stm)
{
    auto *callExpr = dyn_cast<CallExpr>(stm); // Member or operator call
    if (!callExpr) {
        return;
    }

    // For a chain like getList().first(), returns {first(), getList()}
    std::vector<CallExpr *> callExprs = Utils::callListForChain(callExpr); // callExpr would be first()
    if (callExprs.size() < 2) {
        return;
    }

    CallExpr *firstCallToBeEvaluated = callExprs.at(callExprs.size() - 1); // This is the call to getList()
    FunctionDecl *firstFunc = firstCallToBeEvaluated->getDirectCallee();
    if (!firstFunc) {
        return;
    }

    QualType qt = firstFunc->getReturnType();
    const Type *firstFuncReturnType = qt.getTypePtrOrNull();
    if (!firstFuncReturnType) {
        return;
    }

    if (firstFuncReturnType->isReferenceType() || firstFuncReturnType->isPointerType()) {
        return;
    }

    if (qt.isConstQualified()) {
        return; // const doesn't detach
    }

    auto *firstMethod = dyn_cast<CXXMethodDecl>(firstFunc);
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
    auto *detachingMethod = detachingFunc ? dyn_cast<CXXMethodDecl>(detachingFunc) : nullptr;
    const Type *detachingMethodReturnType = detachingMethod ? detachingMethod->getReturnType().getTypePtrOrNull() : nullptr;
    if (!detachingMethod || !detachingMethodReturnType) {
        return;
    }

    // Check if it's one of the implicit shared classes
    const CXXRecordDecl *classDecl = detachingMethod->getParent();
    StringRef className = clazy::name(classDecl);

    const std::unordered_map<std::string, std::vector<StringRef>> &methodsByType = clazy::detachingMethods();
    auto it = methodsByType.find(className.str());
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

    std::string error;

    const bool isReadFunction = clazy::contains(allowedFunctions, functionName);
    const bool isWriteFunction = clazy::contains(allowedWriteFunctions, functionName);

    if (isReadFunction || isWriteFunction) {
        bool returnTypeIsIterator = false;
        if (const CXXRecordDecl *returnRecord = detachingMethodReturnType->getAsCXXRecordDecl()) {
            returnTypeIsIterator = clazy::name(returnRecord) == "iterator";
        }

        if (isWriteFunction && (detachingMethodReturnType->isVoidType() || returnTypeIsIterator)) {
            error = std::string("Modifying temporary container is pointless and it also detaches");
        } else {
            error = std::string("Don't call ") + clazy::qualifiedMethodName(detachingMethod) + std::string("() on temporary");
        }

        std::vector<FixItHint> hints = getFixitHints(className, functionName, callExpr);
        emitWarning(stm->getBeginLoc(), error, hints);
    }
}

bool DetachingTemporary::isDetachingMethod(CXXMethodDecl *method) const
{
    if (!method) {
        return false;
    }

    const CXXRecordDecl *record = method->getParent();
    if (!record) {
        return false;
    }

    if (DetachingBase::isDetachingMethod(method)) {
        return true;
    }

    StringRef className = clazy::name(record);
    if (auto it = m_writeMethodsByType.find(className); it != m_writeMethodsByType.cend()) {
        return clazy::contains(it->second, clazy::name(method));
    }

    return false;
}
