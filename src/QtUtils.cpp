/*
    SPDX-FileCopyrightText: 2015-2016 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "QtUtils.h"
#include "FunctionUtils.h"
#include "HierarchyUtils.h"
#include "MacroUtils.h"
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
    return record && isQtIterableClass(record->getQualifiedNameAsString());
}

std::string trimQtNamespace(const std::string &name, const std::string qtNamespace)
{
    if (!qtNamespace.empty() && name.starts_with(qtNamespace + "::")) {
        std::string nameCopy = name;
        nameCopy.erase(0, qtNamespace.size() + 2);
        return nameCopy;
    }
    return name;
}

const std::vector<StringRef> &clazy::qtContainers()
{
    static const std::vector<StringRef> classes = {
        "QListSpecialMethods",
        "QListSpecialMethodsBase", // Qt6
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
        "QJsonObject",
        "QLinkedList",
    };
    return classes;
}

const std::vector<StringRef> &clazy::qtCOWContainers()
{
    static const std::vector<StringRef> classes = {
        "QListSpecialMethods",
        "QListSpecialMethodsBase", // Qt6
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
        "QJsonObject",
        "QLinkedList",
    };
    return classes;
}

std::unordered_map<std::string, std::vector<StringRef>> clazy::detachingMethods()
{
    static std::unordered_map<std::string, std::vector<StringRef>> map;
    if (map.empty()) {
        const auto &withConst = detachingMethodsWithConstCounterParts();
        map.reserve(withConst.size());
        for (const auto &[qtClassname, methods] : withConst) {
            auto &dst = map[qtClassname];
            dst.reserve(methods.size());
            for (const auto &p : methods)
                dst.emplace_back(p.first);
        }

        map["QVector"].emplace_back("fill");
    }

    return map;
}

std::unordered_map<std::string, std::vector<clazy::MemberConstcounterpartPair>> clazy::detachingMethodsWithConstCounterParts()
{
    static std::unordered_map<std::string, std::vector<std::pair<StringRef, StringRef>>> map;

    if (map.empty()) {
        using Vec = std::vector<clazy::MemberConstcounterpartPair>;
        const Vec seqApi = {
            {"first", "constFirst"},
            {"last", "constLast"},
            {"begin", "cbegin"},
            {"end", "cend"},
            {"rbegin", "crbegin"},
            {"rend", "crend"},
            {"front", "constFirst"},
            {"back", "constLast"},
            {"operator[]", "at"},
        };

        Vec vecApi = seqApi;
        vecApi.emplace_back("data", "constData");

        const Vec mapApi = {
            {"begin", "cbegin"},
            {"end", "cend"},
            {"first", "constFirst"},
            {"find", "constFind"},
            {"last", "constLast"},
            {"operator[]", "at"},
            {"lowerBound", "lowerBound"}, // const overload exists, same name
            {"upperBound", "upperBound"},
            {"keyValueBegin", "constKeyValueBegin"},
            {"keyValueEnd", "constKeyValueEnd"},
        };

        map.emplace("QList", seqApi);
        map.emplace("QVector", vecApi);
        map.emplace("QLinkedList", seqApi);

        Vec stackApi = vecApi;
        stackApi.emplace_back("top", "top");
        map.emplace("QStack", std::move(stackApi));

        Vec queueApi = vecApi;
        queueApi.emplace_back("head", "head");
        map.emplace("QQueue", std::move(queueApi));

        map.emplace("QMap", mapApi);
        map.emplace("QMultiMap", mapApi);

        const Vec hashApi = {{"begin", "cbegin"}, {"end", "cend"}, {"find", "constFind"}, {"operator[]", "value"}};
        map.emplace("QHash", hashApi);
        map.emplace("QMultiHash", hashApi);
        map.emplace("QSet", hashApi);

        const Vec stringApi = {
            {"begin", "cbegin"},
            {"end", "cend"},
            {"rbegin", "crbegin"},
            {"rend", "crend"},
            {"data", "constData"},
            {"operator[]", "at"},
        };
        map.emplace("QString", stringApi);
        map.emplace("QByteArray", stringApi);

        map.emplace("QImage", Vec{{"bits", "constBits"}, {"scanLine", "constScanLine"}});

        map.emplace("QJsonObject", Vec{{"begin", "cbegin"}, {"end", "cend"}, {"operator[]", "value"}, {"find", "constFind"}});
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

bool clazy::isQtCOWIterableClass(clang::CXXRecordDecl *record, const std::string &qtNamespace)
{
    return record && isQtCOWIterableClass(record->getQualifiedNameAsString(), qtNamespace);
}

bool clazy::isQtCOWIterableClass(const std::string &className, const std::string &qtNamespace)
{
    const auto &classes = qtCOWContainers();
    return clazy::contains(classes, trimQtNamespace(className, qtNamespace));
}

bool clazy::isQtIterableClass(StringRef className)
{
    const auto &classes = qtContainers();
    return clazy::contains(classes, className);
}

bool clazy::isQtAssociativeContainer(clang::CXXRecordDecl *record)
{
    return record && isQtAssociativeContainer(record->getNameAsString());
}

bool clazy::isQtAssociativeContainer(StringRef className)
{
    static const std::vector<StringRef> classes = {"QSet", "QMap", "QHash", "QMultiHash", "QMultiMap"};
    return clazy::contains(classes, className);
}

bool clazy::isQObject(const CXXRecordDecl *decl, const std::string &qtNamespace)
{
    return clazy::derivesFrom(decl, clazy::qtNamespaced("QObject", qtNamespace));
}
bool clazy::isSharedPointer(CXXRecordDecl *record, const std::string &qtNamespace)
{
    static const std::vector<std::string> names = {"std::shared_ptr", "QSharedPointer", "boost::shared_ptr"};
    return record ? clazy::contains(names, trimQtNamespace(record->getQualifiedNameAsString(), qtNamespace)) : false;
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

    static const std::vector<StringRef> names = {
        "QHashIterator",
        "QMapIterator",
        "QSetIterator",
        "QListIterator",
        "QVectorIterator", // typedef in Qt6
        "QStringListIterator", // typedef in Qt6
        "QLinkedListIterator", // removed in Qt6
    };

    return clazy::contains(names, clazy::name(record));
}

bool clazy::isJavaIterator(CXXMemberCallExpr *call)
{
    return call && isJavaIterator(call->getRecordDecl());
}

bool clazy::isQtContainer(QualType t)
{
    const CXXRecordDecl *record = clazy::typeAsRecord(t);
    return record && isQtContainer(record);
}

bool clazy::isQtContainer(const CXXRecordDecl *record)
{
    const StringRef typeName = clazy::name(record);
    return std::ranges::any_of(clazy::qtContainers(), [typeName](StringRef container) {
        return container == typeName;
    });
}

bool clazy::isAReserveClass(CXXRecordDecl *recordDecl, const std::string &qtNamespace)
{
    if (!recordDecl) {
        return false;
    }

    static const std::vector<std::string> classes = {
        clazy::qtNamespaced("QVector", qtNamespace),
        "std::vector",
        clazy::qtNamespaced("QList", qtNamespace),
        clazy::qtNamespaced("QSet", qtNamespace),
    };
    return std::ranges::any_of(classes, [recordDecl](const std::string &className) {
        return clazy::derivesFrom(recordDecl, className);
    });
}

clang::CXXRecordDecl *clazy::getQObjectBaseClass(clang::CXXRecordDecl *recordDecl, const std::string &qtNamespace)
{
    if (!recordDecl) {
        return nullptr;
    }

    for (auto baseClass : recordDecl->bases()) {
        if (CXXRecordDecl *record = clazy::recordFromBaseSpecifier(baseClass); isQObject(record, qtNamespace)) {
            return record;
        }
    }

    return nullptr;
}

bool clazy::isConnect(FunctionDecl *func, const std::string &qtNamespace)
{
    return func && func->getQualifiedNameAsString() == clazy::qtNamespaced("QObject::connect", qtNamespace);
}

bool clazy::connectHasPMFStyle(FunctionDecl *func)
{
    // Look for char* arguments
    for (auto *param : Utils::functionParameters(func)) {
        QualType qt = param->getType();
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
    if (auto casted = dyn_cast<ImplicitCastExpr>(expr)) {
        if (auto *declRef = dyn_cast<DeclRefExpr>(casted->getSubExpr())) {
            if (auto *varDecl = dyn_cast<VarDecl>(declRef->getDecl())) {
                auto *varDeclInit = varDecl->getInit();
                // In Qt5, we have multiple DeclRefExprs, only the last one is actually relevant for determining if we have a signal or not
                std::vector<DeclRefExpr *> res;
                clazy::getChilds<DeclRefExpr>(varDeclInit, res);
                if (!res.empty()) {
                    return dyn_cast<CXXMethodDecl>(res.at(res.size() - 1)->getFoundDecl());
                }
            }
        }
    }
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

clang::ValueDecl *clazy::signalReceiverForConnect(clang::CallExpr *call)
{
    if (!call || call->getNumArgs() < 5) {
        return nullptr;
    }

    return clazy::valueDeclForCallArgument(call, 2);
}

clang::ValueDecl *clazy::signalSenderForConnect(clang::CallExpr *call)
{
    return clazy::valueDeclForCallArgument(call, 0);
}

bool clazy::isBootstrapping(const clang::PreprocessorOptions &ppOpts)
{
    return clazy::isPredefined(ppOpts, "QT_BOOTSTRAPPED");
}

bool clazy::isInForeach(const clang::ASTContext *context, clang::SourceLocation loc)
{
    return clazy::isInAnyMacro(context, loc, {"Q_FOREACH", "foreach"});
}
