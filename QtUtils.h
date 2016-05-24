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

#ifndef CLAZY_QT_UTILS_H
#define CLAZY_QT_UTILS_H

#include "clazylib_export.h"

#include <string>
#include <vector>

namespace clang {
class CXXRecordDecl;
class CompilerInstance;
class Type;
class CXXMemberCallExpr;
class CallExpr;
class ValueDecl;
class LangOptions;
}

namespace QtUtils
{

/**
 * Returns true if the class is a Qt class which can be iterated with foreach.
 * Which means all containers and also stuff like QAssociativeIterable.
 */
CLAZYLIB_EXPORT bool isQtIterableClass(clang::CXXRecordDecl *record);

/**
 * Overload.
 */
CLAZYLIB_EXPORT bool isQtIterableClass(const std::string &className);

/**
 * Returns true if the class is a Qt class which can be iterated with foreach and also implicitly shared.
 */
CLAZYLIB_EXPORT bool isQtCOWIterableClass(clang::CXXRecordDecl *record);

/**
 * Overload.
 */
CLAZYLIB_EXPORT bool isQtCOWIterableClass(const std::string &className);

/**
 * Returns true if the class is a Qt class which is an associative container (QHash, QMap, QSet)
 */
CLAZYLIB_EXPORT bool isQtAssociativeContainer(clang::CXXRecordDecl *record);

/**
 * Overload.
 */
CLAZYLIB_EXPORT bool isQtAssociativeContainer(const std::string &className);

/**
 * Returns a list of Qt containers.
 */
CLAZYLIB_EXPORT const std::vector<std::string> & qtContainers();

/**
 * Returns a list of implicitly shared Qt containers.
 */
CLAZYLIB_EXPORT const std::vector<std::string> & qtCOWContainers();

/**
 * Returns true if a type represents a Qt container class.
 */
CLAZYLIB_EXPORT bool isQtContainer(clang::QualType, clang::LangOptions);


/**
 * Returns true if -DQT_BOOTSTRAPPED was passed to the compiler
 */
CLAZYLIB_EXPORT bool isBootstrapping(const clang::CompilerInstance &);

/**
 * Returns if decl is or derives from QObject
 */
CLAZYLIB_EXPORT bool isQObject(clang::CXXRecordDecl *decl);

/**
 * Convertible means that a signal with of type source can connect to a signal/slot of type target
 */
CLAZYLIB_EXPORT bool isConvertibleTo(const clang::Type *source, const clang::Type *target);

/**
 * Returns true if \a loc is in a foreach macro
 */
CLAZYLIB_EXPORT bool isInForeach(const clang::CompilerInstance &ci, clang::SourceLocation loc);

/**
 * Returns true if \a record is a java-style iterator
 */
CLAZYLIB_EXPORT bool isJavaIterator(clang::CXXRecordDecl *record);

CLAZYLIB_EXPORT bool isJavaIterator(clang::CXXMemberCallExpr *call);

/**
 * Returns true if the call is on a java-style iterator class.
 * Returns if sizeof(T) > sizeof(void*), which would make QList<T> inefficient
 */
CLAZYLIB_EXPORT bool isTooBigForQList(clang::QualType, const clang::CompilerInstance &ci);

CLAZYLIB_EXPORT clang::ValueDecl *signalSenderForConnect(clang::CallExpr *call);

}

#endif
