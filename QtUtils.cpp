/*
   This file is part of the clang-lazy static checker.

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

#include "QtUtils.h"
#include "Utils.h"

using namespace std;

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
    return find(classes.cbegin(), classes.cend(), className) != classes.cend();
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
    return find(classes.cbegin(), classes.cend(), className) != classes.cend();
}
