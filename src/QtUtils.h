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

#include "clazy_export.h"

#include "TypeUtils.h"
#include "MacroUtils.h"
#include "FunctionUtils.h"

#include <clang/AST/ASTContext.h>

#include <string>
#include <vector>
#include <unordered_map>

namespace clang {
class CXXRecordDecl;
class CompilerInstance;
class Type;
class CXXMemberCallExpr;
class CallExpr;
class ValueDecl;
class LangOptions;
class QualType;
class VarDecl;
class SourceLocation;
class FunctionDecl;
class UnaryOperator;
class CXXMethodDecl;
class Expr;
}

struct StmtBodyRange;

namespace clazy
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
 * Returns if the iterators belongs to a COW container
 */
inline bool isQtCOWIterator(clang::CXXRecordDecl *itRecord)
{
    if (!itRecord)
        return false;

    auto parent = llvm::dyn_cast_or_null<clang::CXXRecordDecl>(itRecord->getParent());
    return parent && clazy::isQtCOWIterableClass(parent);
}

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
 * Returns a map with the list of method names that detach each container.
 */
CLAZYLIB_EXPORT std::unordered_map<std::string, std::vector<std::string>> detachingMethods();

/**
 * Returns true if a type represents a Qt container class.
 */
CLAZYLIB_EXPORT bool isQtContainer(clang::QualType);


/**
 * Returns true if -DQT_BOOTSTRAPPED was passed to the compiler
 */
inline bool isBootstrapping(const clang::PreprocessorOptions &ppOpts)
{
    return clazy::isPredefined(ppOpts, "QT_BOOTSTRAPPED");
}

/**
 * Returns if decl is or derives from QObject
 */
CLAZYLIB_EXPORT bool isQObject(clang::CXXRecordDecl *decl);

/**
 * Overload.
 */
CLAZYLIB_EXPORT bool isQObject(clang::QualType);

/**
 * Convertible means that a signal with of type source can connect to a signal/slot of type target
 */
CLAZYLIB_EXPORT bool isConvertibleTo(const clang::Type *source, const clang::Type *target);

/**
 * Returns true if \a loc is in a foreach macro
 */
inline bool isInForeach(const clang::ASTContext *context, clang::SourceLocation loc)
{
    return clazy::isInAnyMacro(context, loc, { "Q_FOREACH", "foreach" });
}

/**
 * Returns true if \a record is a java-style iterator
 */
CLAZYLIB_EXPORT bool isJavaIterator(clang::CXXRecordDecl *record);

CLAZYLIB_EXPORT bool isJavaIterator(clang::CXXMemberCallExpr *call);

/**
 * Returns true if the call is on a java-style iterator class.
 * Returns if sizeof(T) > sizeof(void*), which would make QList<T> inefficient
 */
inline bool isTooBigForQList(clang::QualType qt, const clang::ASTContext *context)
{
    return (int)context->getTypeSize(qt) <= TypeUtils::sizeOfPointer(context, qt);
}

/**
 * Returns true if a class has a ctor that has a parameter of type paramType.
 * ok will be false if an error occurred, or if the record is a fwd declaration, which isn't enough
 * for we to find out the signature.
 * numCtors will have the number of constructors analyized.
 */
bool recordHasCtorWithParam(clang::CXXRecordDecl *record, const std::string &paramType, bool &ok, int &numCtors);

/**
 * Returns true if we can prove the container doesn't detach.
 * Returns false otherwise, meaning that you can't conclude anything if false is returned.
 *
 * For true to be returned, all these conditions must verify:
 * - Container is a local variable
 * - It's not passed to any function
 * - It's not assigned to another variable
 */
CLAZYLIB_EXPORT bool containerNeverDetaches(const clang::VarDecl *varDecl,
                                            StmtBodyRange bodyRange);

/**
 * Returns true if recordDecl is one of the container classes that supports reserve(), such
 * as QList, QVector, etc.
 */
CLAZYLIB_EXPORT bool isAReserveClass(clang::CXXRecordDecl *recordDecl);

/**
 * Returns the base class that inherits QObject.
 * Useful when the class has more than one base class and we're only interested in the QObject one.
 */
CLAZYLIB_EXPORT clang::CXXRecordDecl *getQObjectBaseClass(clang::CXXRecordDecl *recordDecl);

/**
 * Returns true if the function declaration is QObject::connect().
 */
CLAZYLIB_EXPORT bool isConnect(clang::FunctionDecl *func);

/**
 * Returns true if the function declaration represents a QObject::connect() using the new Qt5
 * (pointer to member) syntax.
 *
 * It's assumed that func represents a connect().
 */
CLAZYLIB_EXPORT bool connectHasPMFStyle(clang::FunctionDecl *func);


/**
 * Returns the method referenced by a PMF-style connect for the specified connect() call.
 */
CLAZYLIB_EXPORT clang::CXXMethodDecl* pmfFromConnect(clang::CallExpr *funcCall, int argIndex);

CLAZYLIB_EXPORT clang::CXXMethodDecl* pmfFromUnary(clang::Expr *e);
CLAZYLIB_EXPORT clang::CXXMethodDecl* pmfFromUnary(clang::UnaryOperator *uo);

/**
 * Returns the varDecl for the 1st argument in a connect call
 */
inline clang::ValueDecl *signalSenderForConnect(clang::CallExpr *call)
{
    return clazy::valueDeclForCallArgument(call, 0);
}

/**
 * Returns the varDecl for 3rd argument in connects that are passed an explicit
 * receiver or context QObject.
 */
inline clang::ValueDecl *signalReceiverForConnect(clang::CallExpr *call)
{
    if (!call || call->getNumArgs() < 5)
        return nullptr;

    return clazy::valueDeclForCallArgument(call, 3);
}

/**
 * Returns the receiver method, in a PMF connect statement.
 * The method can be a slot or a signal. If it's a lambda or functor nullptr is returned
 */
inline clang::CXXMethodDecl* receiverMethodForConnect(clang::CallExpr *call)
{

    clang::CXXMethodDecl *receiverMethod = clazy::pmfFromConnect(call, 2);
    if (receiverMethod)
        return receiverMethod;

    // It's either third or fourth argument
    return clazy::pmfFromConnect(call, 3);
}

}

#endif
