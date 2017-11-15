/*
   This file is part of the clazy static checker.

  Copyright (C) 2015-2016 Sergio Martins <smartins@kde.org>

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

#include "clazy_stl.h"
#include "QtUtils.h"
#include "Utils.h"
#include "TypeUtils.h"
#include "StmtBodyRange.h"
#include "MacroUtils.h"
#include "HierarchyUtils.h"
#include "StringUtils.h"
#include "ContextUtils.h"

#include <clang/AST/AST.h>

using namespace std;
using namespace clang;

bool QtUtils::isQtIterableClass(clang::CXXRecordDecl *record)
{
    if (!record)
        return false;

    return isQtIterableClass(record->getQualifiedNameAsString());
}

const vector<string> & QtUtils::qtContainers()
{
    static const vector<string> classes = { "QListSpecialMethods", "QList", "QVector", "QVarLengthArray", "QMap",
                                            "QHash", "QMultiMap", "QMultiHash", "QSet", "QStack", "QQueue", "QString", "QStringRef",
                                            "QByteArray", "QSequentialIterable", "QAssociativeIterable", "QJsonArray", "QLinkedList" };
    return classes;
}

const vector<string> & QtUtils::qtCOWContainers()
{
    static const vector<string> classes = { "QListSpecialMethods", "QList", "QVector", "QMap", "QHash",
                                            "QMultiMap", "QMultiHash", "QSet", "QStack", "QQueue", "QString", "QStringRef",
                                            "QByteArray", "QJsonArray", "QLinkedList" };
    return classes;
}


std::unordered_map<string, std::vector<string> > QtUtils::detachingMethods()
{
    static std::unordered_map<string, std::vector<string> > map;
    if (map.empty()) {
        map["QList"] = {"first", "last", "begin", "end", "front", "back", "operator[]"};
        map["QVector"] = {"first", "last", "begin", "end", "front", "back", "data", "operator[]", "fill" };
        map["QMap"] = {"begin", "end", "first", "find", "last", "lowerBound", "upperBound", "operator[]" };
        map["QHash"] = {"begin", "end", "find", "operator[]" };
        map["QLinkedList"] = {"first", "last", "begin", "end", "front", "back", "operator[]" };
        map["QSet"] = {"begin", "end", "find", "operator[]" };
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


bool QtUtils::isQtCOWIterableClass(clang::CXXRecordDecl *record)
{
    if (!record)
        return false;

    return isQtCOWIterableClass(record->getQualifiedNameAsString());
}

bool QtUtils::isQtCOWIterableClass(const string &className)
{
    const auto &classes = qtCOWContainers();
    return clazy_std::contains(classes, className);
}

bool QtUtils::isQtIterableClass(const string &className)
{
    const auto &classes = qtContainers();
    return clazy_std::contains(classes, className);
}

bool QtUtils::isQtAssociativeContainer(clang::CXXRecordDecl *record)
{
    if (!record)
        return false;

    return isQtAssociativeContainer(record->getNameAsString());
}

bool QtUtils::isQtAssociativeContainer(const string &className)
{
    static const vector<string> classes = { "QSet", "QMap", "QHash" };
    return clazy_std::contains(classes, className);
}

bool QtUtils::isQObject(CXXRecordDecl *decl)
{
    return TypeUtils::derivesFrom(decl, "QObject");
}

bool QtUtils::isQObject(clang::QualType qt)
{
    qt = TypeUtils::pointeeQualType(qt);
    const auto t = qt.getTypePtrOrNull();
    return t ? isQObject(t->getAsCXXRecordDecl()) : false;
}

bool QtUtils::isConvertibleTo(const Type *source, const Type *target)
{
    if (!source || !target)
        return false;

    if (source->isPointerType() ^ target->isPointerType())
        return false;

    if (source == target)
        return true;

    if (source->getPointeeCXXRecordDecl() && source->getPointeeCXXRecordDecl() == target->getPointeeCXXRecordDecl())
        return true;

    if (source->isIntegerType() && target->isIntegerType())
        return true;

    if (source->isFloatingType() && target->isFloatingType())
        return true;

    // "QString" can convert to "const QString &" and vice versa
    if (TypeUtils::isConstRef(source) && source->getPointeeType().getTypePtrOrNull() == target)
        return true;
    if (TypeUtils::isConstRef(target) && target->getPointeeType().getTypePtrOrNull() == source)
        return true;

    return false;
}

bool QtUtils::isJavaIterator(CXXRecordDecl *record)
{
    if (!record)
        return false;

    static const vector<string> names = { "QHashIterator", "QMapIterator", "QSetIterator", "QListIterator",
                                         "QVectorIterator", "QLinkedListIterator", "QStringListIterator" };

    return clazy_std::contains(names, record->getNameAsString());
}

bool QtUtils::isJavaIterator(CXXMemberCallExpr *call)
{
    if (!call)
        return false;

    return isJavaIterator(call->getRecordDecl());
}

bool QtUtils::isQtContainer(QualType t)
{
    CXXRecordDecl *record = TypeUtils::typeAsRecord(t);
    if (!record)
        return false;

    const string typeName = record->getNameAsString();
    return clazy_std::any_of(QtUtils::qtContainers(), [typeName] (const string &container) {
        return container == typeName;
    });
}

bool QtUtils::containerNeverDetaches(const clang::VarDecl *valDecl, StmtBodyRange bodyRange) // clazy:exclude=function-args-by-value
{
    if (!valDecl)
        return false;

    const FunctionDecl *context = dyn_cast<FunctionDecl>(valDecl->getDeclContext());
    if (!context)
        return false;

    bodyRange.body = context->getBody();
    if (!bodyRange.body)
        return false;

    // TODO1: Being passed to a function as const should be OK
    if (Utils::isPassedToFunction(bodyRange, valDecl, false))
        return false;

    return true;
}

bool QtUtils::isAReserveClass(CXXRecordDecl *recordDecl)
{
    if (!recordDecl)
        return false;

    static const std::vector<std::string> classes = {"QVector", "std::vector", "QList", "QSet"};

    return clazy_std::any_of(classes, [recordDecl](const string &className) {
        return TypeUtils::derivesFrom(recordDecl, className);
    });
}

clang::CXXRecordDecl *QtUtils::getQObjectBaseClass(clang::CXXRecordDecl *recordDecl)
{
    if (!recordDecl)
        return nullptr;

    for (auto baseClass : recordDecl->bases()) {
        CXXRecordDecl *record = TypeUtils::recordFromBaseSpecifier(baseClass);
        if (isQObject(record))
            return record;
    }

    return nullptr;
}


bool QtUtils::isConnect(FunctionDecl *func)
{
    return func && func->getQualifiedNameAsString() == "QObject::connect";
}

bool QtUtils::connectHasPMFStyle(FunctionDecl *func)
{
    // Look for char* arguments
    for (auto parm : Utils::functionParameters(func)) {
        QualType qt = parm->getType();
        const Type *t = qt.getTypePtrOrNull();
        if (!t || !t->isPointerType())
            continue;

        const Type *ptt = t->getPointeeType().getTypePtrOrNull();
        if (ptt && ptt->isCharType())
            return false;
    }

    return true;
}

CXXMethodDecl *QtUtils::pmfFromConnect(CallExpr *funcCall, int argIndex)
{
    if (!funcCall)
        return nullptr;

    const int numArgs = funcCall->getNumArgs();
    if (numArgs < 3) {
        llvm::errs() << "error, connect call has less than 3 arguments\n";
        return nullptr;
    }

    if (argIndex >= numArgs)
        return nullptr;

    Expr *expr = funcCall->getArg(argIndex);
    return pmfFromUnary(expr);
}


CXXMethodDecl *QtUtils::pmfFromUnary(Expr *expr)
{
    if (auto uo = dyn_cast<UnaryOperator>(expr)) {
        return pmfFromUnary(uo);
    } else if (auto call = dyn_cast<CXXOperatorCallExpr>(expr)) {

        if (call->getNumArgs() <= 1)
            return nullptr;

        FunctionDecl *func = call->getDirectCallee();
        if (!func)
            return nullptr;

        auto context = func->getParent();
        if (!context)
            return nullptr;

        CXXRecordDecl *record = dyn_cast<CXXRecordDecl>(context);
        if (!record)
            return nullptr;

        const std::string className = record->getQualifiedNameAsString();
        if (className != "QNonConstOverload" && className != "QConstOverload")
            return nullptr;

        return pmfFromUnary(dyn_cast<UnaryOperator>(call->getArg(1)));
    } else if (auto staticCast = dyn_cast<CXXStaticCastExpr>(expr)) {
        return pmfFromUnary(staticCast->getSubExpr());
    }

    return nullptr;
}

CXXMethodDecl *QtUtils::pmfFromUnary(UnaryOperator *uo)
{
    if (!uo)
        return nullptr;

    Expr *subExpr = uo->getSubExpr();
    if (!subExpr)
        return nullptr;

    auto declref = dyn_cast<DeclRefExpr>(subExpr);

    if (declref)
        return dyn_cast<CXXMethodDecl>(declref->getDecl());

    return nullptr;
}

bool QtUtils::recordHasCtorWithParam(clang::CXXRecordDecl *record, const std::string &paramType, bool &ok, int &numCtors)
{
    ok = true;
    numCtors = 0;
    if (!record || !record->hasDefinition() ||
        record->getDefinition() != record) { // Means fwd decl
        ok = false;
        return false;
    }

    for (auto ctor : record->ctors()) {
        if (ctor->isCopyOrMoveConstructor())
            continue;
        numCtors++;
        for (auto param : ctor->parameters()) {
            QualType qt = TypeUtils::pointeeQualType(param->getType());
            if (!qt.isConstQualified() && TypeUtils::derivesFrom(qt, paramType)) {
                return true;
            }
        }
    }

    return false;
}
