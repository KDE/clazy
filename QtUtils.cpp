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
                                            "QHash", "QMultiMap", "QMultiHash", "QSet", "QStack", "QQueue", "QString",
                                            "QByteArray", "QSequentialIterable", "QAssociativeIterable", "QJsonArray", "QLinkedList" };
    return classes;
}

const vector<string> & QtUtils::qtCOWContainers()
{
    static const vector<string> classes = { "QListSpecialMethods", "QList", "QVector", "QVarLengthArray", "QMap",
                                            "QHash", "QMultiMap", "QMultiHash", "QSet", "QStack", "QQueue", "QString",
                                            "QByteArray", "QJsonArray", "QLinkedList" };
    return classes;
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

bool QtUtils::isBootstrapping(const clang::CompilerInstance &ci)
{
    return MacroUtils::isPredefined(ci, "QT_BOOTSTRAPPED");
}

bool QtUtils::isQObject(CXXRecordDecl *decl)
{
    if (!decl || !decl->hasDefinition())
        return false;

    if (decl->getName() == "QObject")
        return true;

    return clazy_std::any_of(decl->bases(), [](CXXBaseSpecifier base) {
        const Type *type = base.getType().getTypePtr();
        return type && QtUtils::isQObject(type->getAsCXXRecordDecl());
    });
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

    return false;
}

bool QtUtils::isInForeach(const clang::CompilerInstance &ci, clang::SourceLocation loc)
{
    return MacroUtils::isInAnyMacro(ci, loc, { "Q_FOREACH", "foreach" });
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

clang::ValueDecl *QtUtils::signalSenderForConnect(clang::CallExpr *call)
{
    if (!call || call->getNumArgs() < 1)
        return nullptr;

    Expr *firstArg = call->getArg(0);
    auto declRef = isa<DeclRefExpr>(firstArg) ? cast<DeclRefExpr>(firstArg) : HierarchyUtils::getFirstChildOfType2<DeclRefExpr>(firstArg);
    if (!declRef)
        return nullptr;

    return declRef->getDecl();
}

bool QtUtils::isTooBigForQList(clang::QualType qt, const clang::CompilerInstance &ci)
{
    return (int)ci.getASTContext().getTypeSize(qt) <= TypeUtils::sizeOfPointer(ci, qt);
}

bool QtUtils::isQtContainer(QualType t, LangOptions lo)
{
    const string typeName = StringUtils::simpleTypeName(t, lo);
    return clazy_std::any_of(QtUtils::qtContainers(), [typeName] (const string &container) {
        return container == typeName;
    });
}

bool QtUtils::containerNeverDetaches(const clang::VarDecl *valDecl, StmtBodyRange bodyRange)
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

    static const std::vector<std::string> classes = {"QVector", "vector", "QList", "QSet"};

    return clazy_std::any_of(classes, [recordDecl](const string &className) {
        return TypeUtils::derivesFrom(recordDecl, className);
    });
}
