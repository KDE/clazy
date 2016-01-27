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
#include "MacroUtils.h"

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
    if (!decl)
        return false;

    if (decl->getName() == "QObject")
        return true;

    return clazy_std::any_of(decl->bases(), [](CXXBaseSpecifier base) {
        const Type *type = base.getType().getTypePtr();
        return type && QtUtils::isQObject(type->getAsCXXRecordDecl());
    });
}
