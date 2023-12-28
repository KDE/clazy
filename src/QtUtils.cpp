/*
    SPDX-FileCopyrightText: 2015-2016 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "QtUtils.h"
#include "StmtBodyRange.h"
#include "StringUtils.h"
#include "TypeUtils.h"
#include "Utils.h"
#include "clazy_stl.h"

#include <clang/AST/ExprCXX.h>
#include <clang/Basic/LLVM.h>
#include <llvm/ADT/ArrayRef.h>
#include <llvm/Support/raw_ostream.h>

using namespace clang;

bool clazy::isQtIterableClass(clang::CXXRecordDecl *record)
{
    if (!record) {
        return false;
    }

    return isQtIterableClass(record->getQualifiedNameAsString());
}

const std::vector<StringRef> &clazy::qtContainers()
{
    static const std::vector<StringRef> classes = {"QListSpecialMethods",
                                                   "QList",
                                                   "QVector",
                                                   "QVarLengthArray",
                                                   "QMap",
                                                   "QHash",
                                                   "QMultiMap",
                                                   "QMultiHash",
                                                   "QSet",
                                                   "QStack",
                                                   "QQueue",
                                                   "QString",
                                                   "QStringRef",
                                                   "QByteArray",
                                                   "QSequentialIterable",
                                                   "QAssociativeIterable",
                                                   "QJsonArray",
                                                   "QLinkedList"};
    return classes;
}

const std::vector<StringRef> &clazy::qtCOWContainers()
{
    static const std::vector<StringRef> classes = {"QListSpecialMethods",
                                                   "QList",
                                                   "QVector",
                                                   "QMap",
                                                   "QHash",
                                                   "QMultiMap",
                                                   "QMultiHash",
                                                   "QSet",
                                                   "QStack",
                                                   "QQueue",
                                                   "QString",
                                                   "QStringRef",
                                                   "QByteArray",
                                                   "QJsonArray",
                                                   "QLinkedList"};
    return classes;
}

std::unordered_map<std::string, std::vector<StringRef>> clazy::detachingMethods()
{
    static std::unordered_map<std::string, std::vector<StringRef>> map;
    if (map.empty()) {
        map = detachingMethodsWithConstCounterParts();
        map["QVector"].push_back("fill");
    }

    return map;
}

std::unordered_map<std::string, std::vector<StringRef>> clazy::detachingMethodsWithConstCounterParts()
{
    static std::unordered_map<std::string, std::vector<StringRef>> map;
    if (map.empty()) {
        map["QList"] = {"first", "last", "begin", "end", "front", "back", "operator[]"};
        map["QVector"] = {"first", "last", "begin", "end", "front", "back", "data", "operator[]"};
        map["QMap"] = {"begin", "end", "first", "find", "last", "operator[]", "lowerBound", "upperBound"};
        map["QHash"] = {"begin", "end", "find", "operator[]"};
        map["QLinkedList"] = {"first", "last", "begin", "end", "front", "back", "operator[]"};
        map["QSet"] = {"begin", "end", "find", "operator[]"};
        map["QStack"] = map["QVector"];
        map["QStack"].push_back({"top"});
        map["QQueue"] = map["QVector"];
        map["QQueue"].push_back({"head"});
        map["QMultiMap"] = map["QMap"];
        map["QMultiHash"] = map["QHash"];
        map["QString"] = {"begin", "end", "data", "operator[]"};
        map["QByteArray"] = {"data", "operator[]"};
        map["QImage"] = {"bits", "scanLine"};
    }

    return map;
}

bool clazy::isQMetaMethod(CallExpr *Call, unsigned int argIndex)
{
    Expr *arg = Call->getArg(argIndex);
    QualType type = arg->getType();
    if (!type->isRecordType()) {
        return false;
    }

    CXXRecordDecl *recordDecl = type->getAsCXXRecordDecl();
    if (!recordDecl) {
        return false;
    }

    return recordDecl->getQualifiedNameAsString() == "QMetaMethod";
}

bool clazy::isQtCOWIterableClass(clang::CXXRecordDecl *record)
{
    if (!record) {
        return false;
    }

    return isQtCOWIterableClass(record->getQualifiedNameAsString());
}

bool clazy::isQtCOWIterableClass(const std::string &className)
{
    const auto &classes = qtCOWContainers();
    return clazy::contains(classes, className);
}

bool clazy::isQtIterableClass(StringRef className)
{
    const auto &classes = qtContainers();
    return clazy::contains(classes, className);
}

bool clazy::isQtAssociativeContainer(clang::CXXRecordDecl *record)
{
    if (!record) {
        return false;
    }

    return isQtAssociativeContainer(record->getNameAsString());
}

bool clazy::isQtAssociativeContainer(StringRef className)
{
    static const std::vector<StringRef> classes = {"QSet", "QMap", "QHash"};
    return clazy::contains(classes, className);
}

bool clazy::isQObject(const CXXRecordDecl *decl)
{
    return clazy::derivesFrom(decl, "QObject");
}

bool clazy::isQObject(clang::QualType qt)
{
    qt = clazy::pointeeQualType(qt);
    const auto *const t = qt.getTypePtrOrNull();
    return t ? isQObject(t->getAsCXXRecordDecl()) : false;
}

bool clazy::isConvertibleTo(const Type *source, const Type *target)
{
    if (!source || !target) {
        return false;
    }

    if (source->isPointerType() ^ target->isPointerType()) {
        return false;
    }

    if (source == target) {
        return true;
    }

    if (source->getPointeeCXXRecordDecl() && source->getPointeeCXXRecordDecl() == target->getPointeeCXXRecordDecl()) {
        return true;
    }

    if (source->isIntegerType() && target->isIntegerType()) {
        return true;
    }

    if (source->isFloatingType() && target->isFloatingType()) {
        return true;
    }

    // "QString" can convert to "const QString &" and vice versa
    if (clazy::isConstRef(source) && source->getPointeeType().getTypePtrOrNull() == target) {
        return true;
    }
    if (clazy::isConstRef(target) && target->getPointeeType().getTypePtrOrNull() == source) {
        return true;
    }

    return false;
}

bool clazy::isJavaIterator(CXXRecordDecl *record)
{
    if (!record) {
        return false;
    }

    static const std::vector<StringRef> names =
        {"QHashIterator", "QMapIterator", "QSetIterator", "QListIterator", "QVectorIterator", "QLinkedListIterator", "QStringListIterator"};

    return clazy::contains(names, clazy::name(record));
}

bool clazy::isJavaIterator(CXXMemberCallExpr *call)
{
    if (!call) {
        return false;
    }

    return isJavaIterator(call->getRecordDecl());
}

bool clazy::isQtContainer(QualType t)
{
    CXXRecordDecl *record = clazy::typeAsRecord(t);
    if (!record) {
        return false;
    }

    return isQtContainer(record);
}

bool clazy::isQtContainer(const CXXRecordDecl *record)
{
    const StringRef typeName = clazy::name(record);
    return clazy::any_of(clazy::qtContainers(), [typeName](StringRef container) {
        return container == typeName;
    });
}

bool clazy::isAReserveClass(CXXRecordDecl *recordDecl)
{
    if (!recordDecl) {
        return false;
    }

    static const std::vector<std::string> classes = {"QVector", "std::vector", "QList", "QSet"};

    return clazy::any_of(classes, [recordDecl](const std::string &className) {
        return clazy::derivesFrom(recordDecl, className);
    });
}

clang::CXXRecordDecl *clazy::getQObjectBaseClass(clang::CXXRecordDecl *recordDecl)
{
    if (!recordDecl) {
        return nullptr;
    }

    for (auto baseClass : recordDecl->bases()) {
        CXXRecordDecl *record = clazy::recordFromBaseSpecifier(baseClass);
        if (isQObject(record)) {
            return record;
        }
    }

    return nullptr;
}

bool clazy::isConnect(FunctionDecl *func)
{
    return func && func->getQualifiedNameAsString() == "QObject::connect";
}

bool clazy::connectHasPMFStyle(FunctionDecl *func)
{
    // Look for char* arguments
    for (auto *parm : Utils::functionParameters(func)) {
        QualType qt = parm->getType();
        const Type *t = qt.getTypePtrOrNull();
        if (!t || !t->isPointerType()) {
            continue;
        }

        const Type *ptt = t->getPointeeType().getTypePtrOrNull();
        if (ptt && ptt->isCharType()) {
            return false;
        }
    }

    return true;
}

CXXMethodDecl *clazy::pmfFromConnect(CallExpr *funcCall, int argIndex)
{
    if (!funcCall) {
        return nullptr;
    }

    const int numArgs = funcCall->getNumArgs();
    if (numArgs < 3) {
        llvm::errs() << "error, connect call has less than 3 arguments\n";
        return nullptr;
    }

    if (argIndex >= numArgs) {
        return nullptr;
    }

    Expr *expr = funcCall->getArg(argIndex);
    return pmfFromExpr(expr);
}

CXXMethodDecl *clazy::pmfFromExpr(Expr *expr)
{
    if (auto *uo = dyn_cast<UnaryOperator>(expr)) {
        return pmfFromUnary(uo);
    }
    if (auto *call = dyn_cast<CXXOperatorCallExpr>(expr)) {
        if (call->getNumArgs() <= 1) {
            return nullptr;
        }

        FunctionDecl *func = call->getDirectCallee();
        if (!func) {
            return nullptr;
        }

        auto *context = func->getParent();
        if (!context) {
            return nullptr;
        }

        auto *record = dyn_cast<CXXRecordDecl>(context);
        if (!record) {
            return nullptr;
        }

        const std::string className = record->getQualifiedNameAsString();
        if (className != "QNonConstOverload" && className != "QConstOverload") {
            return nullptr;
        }

        return pmfFromUnary(dyn_cast<UnaryOperator>(call->getArg(1)));
    } else if (auto *staticCast = dyn_cast<CXXStaticCastExpr>(expr)) {
        return pmfFromExpr(staticCast->getSubExpr());
    } else if (auto *callexpr = dyn_cast<CallExpr>(expr)) {
        // QOverload case, go deeper one level to get to the UnaryOperator
        if (callexpr->getNumArgs() == 1) {
            return pmfFromExpr(callexpr->getArg(0));
        }
    } else if (auto *matTempExpr = dyn_cast<MaterializeTemporaryExpr>(expr)) {
        // Qt6 PMF, go deeper one level to get to the UnaryOperator
        return pmfFromExpr(matTempExpr->getSubExpr());
    } else if (auto *impl = dyn_cast<ImplicitCastExpr>(expr)) {
        return pmfFromExpr(impl->getSubExpr());
    }

    return nullptr;
}

CXXMethodDecl *clazy::pmfFromUnary(UnaryOperator *uo)
{
    if (!uo) {
        return nullptr;
    }

    Expr *subExpr = uo->getSubExpr();
    if (!subExpr) {
        return nullptr;
    }

    auto *declref = dyn_cast<DeclRefExpr>(subExpr);

    if (declref) {
        return dyn_cast<CXXMethodDecl>(declref->getDecl());
    }

    return nullptr;
}

bool clazy::recordHasCtorWithParam(clang::CXXRecordDecl *record, const std::string &paramType, bool &ok, int &numCtors)
{
    ok = true;
    numCtors = 0;
    if (!record || !record->hasDefinition() || record->getDefinition() != record) { // Means fwd decl
        ok = false;
        return false;
    }

    for (auto *ctor : record->ctors()) {
        if (ctor->isCopyOrMoveConstructor()) {
            continue;
        }
        numCtors++;
        for (auto *param : ctor->parameters()) {
            QualType qt = clazy::pointeeQualType(param->getType());
            if (!qt.isConstQualified() && clazy::derivesFrom(qt, paramType)) {
                return true;
            }
        }
    }

    return false;
}
