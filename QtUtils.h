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

#ifndef CLAZY_QT_UTILS_H
#define CLAZY_QT_UTILS_H

#include <string>
#include <vector>

namespace clang {
class CXXRecordDecl;
}

namespace QtUtils
{

/**
 * Returns true if the class is a Qt class which can be iterated with foreach.
 * Which means all containers and also stuff like QAssociativeIterable.
 */
bool isQtIterableClass(clang::CXXRecordDecl *record);

/**
 * Overload.
 */
bool isQtIterableClass(const std::string &className);

/**
 * Returns true if the class is a Qt class which is an associative container (QHash, QMap, QSet)
 */
bool isQtAssociativeContainer(clang::CXXRecordDecl *record);

/**
 * Overload.
 */
bool isQtAssociativeContainer(const std::string &className);

/**
 * Returns a list of Qt containers.
 */
const std::vector<std::string> & qtContainers();

}

#endif
