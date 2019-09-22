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

#include "TypeUtils.h"
#include "MacroUtils.h"
#include "FunctionUtils.h"
#include "StringUtils.h"
#include "Utils.h"
#include "clazy_stl.h"

#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/DeclTemplate.h>
#include <clang/AST/Expr.h>
#include <clang/AST/OperationKinds.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/TemplateBase.h>
#include <clang/AST/Type.h>
#include <clang/Basic/SourceLocation.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Casting.h>

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
class PreprocessorOptions;
class SourceManager;
}

struct StmtBodyRange;

namespace clazy
{

/**
 * Returns true if the class is a Qt class which can be iterated with foreach.
 * Which means all containers and also stuff like QAssociativeIterable.
 */
bool isQtIterableClass(clang::CXXRecordDecl *record);

/**
 * Overload.
 */
bool isQtIterableClass(llvm::StringRef className);

/**
 * Returns true if the class is a Qt class which can be iterated with foreach and also implicitly shared.
 */
bool isQtCOWIterableClass(clang::CXXRecordDecl *record);

/**
 * Overload.
 */
bool isQtCOWIterableClass(const std::string &className);

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
bool isQtAssociativeContainer(clang::CXXRecordDecl *record);

/**
 * Overload.
 */
bool isQtAssociativeContainer(llvm::StringRef className);

/**
 * Returns a list of Qt containers.
 */
const std::vector<llvm::StringRef> & qtContainers();

/**
 * Returns a list of implicitly shared Qt containers.
 */
const std::vector<llvm::StringRef> & qtCOWContainers();

/**
 * Returns a map with the list of method names that detach each container.
 */
std::unordered_map<std::string, std::vector<llvm::StringRef>> detachingMethods();

/**
 * Returns a map with the list of method names that detach each container, but only those methods
 * with const counterparts.
 */
std::unordered_map<std::string, std::vector<llvm::StringRef>> detachingMethodsWithConstCounterParts();

/**
 * Returns true if a type represents a Qt container class.
 */
bool isQtContainer(clang::QualType);

bool isQtContainer(const clang::CXXRecordDecl *);


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
bool isQObject(const clang::CXXRecordDecl *decl);

/**
 * Overload.
 */
bool isQObject(clang::QualType);

/**
 * Convertible means that a signal with of type source can connect to a signal/slot of type target
 */
bool isConvertibleTo(const clang::Type *source, const clang::Type *target);

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
bool isJavaIterator(clang::CXXRecordDecl *record);

bool isJavaIterator(clang::CXXMemberCallExpr *call);

/**
 * Returns true if the call is on a java-style iterator class.
 * Returns if sizeof(T) > sizeof(void*), which would make QList<T> inefficient
 */
inline bool isTooBigForQList(clang::QualType qt, const clang::ASTContext *context)
{
    return (int)context->getTypeSize(qt) <= clazy::sizeOfPointer(context, qt);
}

/**
 * Returns true if a class has a ctor that has a parameter of type paramType.
 * ok will be false if an error occurred, or if the record is a fwd declaration, which isn't enough
 * for we to find out the signature.
 * numCtors will have the number of constructors analyized.
 */
bool recordHasCtorWithParam(clang::CXXRecordDecl *record, const std::string &paramType, bool &ok, int &numCtors);

/**
 * Returns true if recordDecl is one of the container classes that supports reserve(), such
 * as QList, QVector, etc.
 */
bool isAReserveClass(clang::CXXRecordDecl *recordDecl);

/**
 * Returns the base class that inherits QObject.
 * Useful when the class has more than one base class and we're only interested in the QObject one.
 */
clang::CXXRecordDecl *getQObjectBaseClass(clang::CXXRecordDecl *recordDecl);

/**
 * Returns true if the function declaration is QObject::connect().
 */
bool isConnect(clang::FunctionDecl *func);

/**
 * Returns true if the function declaration represents a QObject::connect() using the new Qt5
 * (pointer to member) syntax.
 *
 * It's assumed that func represents a connect().
 */
bool connectHasPMFStyle(clang::FunctionDecl *func);


/**
 * Returns the method referenced by a PMF-style connect for the specified connect() call.
 */
clang::CXXMethodDecl* pmfFromConnect(clang::CallExpr *funcCall, int argIndex);

clang::CXXMethodDecl* pmfFromUnary(clang::Expr *e);
clang::CXXMethodDecl* pmfFromUnary(clang::UnaryOperator *uo);

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


// Returns if callExpr is a call to qobject_cast()
inline bool is_qobject_cast(clang::Stmt *s, clang::CXXRecordDecl **castTo = nullptr,
                            clang::CXXRecordDecl **castFrom = nullptr)
{
    if (auto callExpr = llvm::dyn_cast<clang::CallExpr>(s)) {
        clang::FunctionDecl *func = callExpr->getDirectCallee();
        if (!func || clazy::name(func) != "qobject_cast")
            return false;

        if (castFrom) {
            clang::Expr *expr = callExpr->getArg(0);
            if (auto implicitCast = llvm::dyn_cast<clang::ImplicitCastExpr>(expr)) {
                if (implicitCast->getCastKind() == clang::CK_DerivedToBase) {
                    expr = implicitCast->getSubExpr();
                }
            }
            clang::QualType qt = clazy::pointeeQualType(expr->getType());
            if (!qt.isNull()) {
                clang::CXXRecordDecl *record = qt->getAsCXXRecordDecl();
                *castFrom = record ? record->getCanonicalDecl() : nullptr;
            }
        }

        if (castTo) {
            auto templateArgs = func->getTemplateSpecializationArgs();
            if (templateArgs->size() == 1) {
                const clang::TemplateArgument &arg = templateArgs->get(0);
                clang::QualType qt = clazy::pointeeQualType(arg.getAsType());
                if (!qt.isNull()) {
                    clang::CXXRecordDecl *record = qt->getAsCXXRecordDecl();
                    *castTo = record ? record->getCanonicalDecl() : nullptr;
                }
            }
        }
        return true;
    }

    return false;
}

inline bool isUIFile(clang::SourceLocation loc, const clang::SourceManager &sm)
{
    const std::string filename = Utils::filenameForLoc(loc, sm);
    return clazy::startsWith(filename, "ui_") && clazy::endsWith(filename, ".h");
}

}

#endif
