/*
  This file is part of the clazy static checker.

  Copyright (C) 2016 Sergio Martins <smartins@kde.org>

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

#ifndef CLAZY_CONTEXT_UTILS_H
#define CLAZY_CONTEXT_UTILS_H

#include "clazylib_export.h"

#include <clang/AST/DeclBase.h>
#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/Stmt.h>

#include <vector>
#include <string>

namespace clang {
class ValueDecl;
class DeclContext;
class SourceManager;
class SourceLocation;
class CXXMethodDecl;
class ParentMap;
}

namespace ContextUtils
{

/**
 * Returns true if a decl is inside a function, instead of say a class field.
 * This returns true if "QList<int> l;" is a local variable, instead of being a class field such
 * as struct Foo { QList<int> l; }
 */
CLAZYLIB_EXPORT bool isValueDeclInFunctionContext(clang::ValueDecl *);

/**
 * Returns the list of scopes for a decl context (namespaces, classes, inner classes, etc)
 * The inner ones are at the beginning of the list
 */
CLAZYLIB_EXPORT std::vector<clang::DeclContext *> contextsForDecl(clang::DeclContext *);


/**
 * Returns the first context of type T in which the specified context is in.
 * Contexts are namespaces, classes, inner classes, functions, etc.

 */
template <typename T>
T* firstContextOfType(clang::DeclContext *context)
{
    if (!context)
        return nullptr;

    if (llvm::isa<T>(context))
        return llvm::cast<T>(context);

    return ContextUtils::firstContextOfType<T>(context->getParent());
}


/**
 * Returns fully/semi-fully qualified name for a method, but doesn't over-qualify with namespaces
 * which we're already in.
 *
 * if currentScope == nullptr will return a fully qualified name
 */

CLAZYLIB_EXPORT std::string getMostNeededQualifiedName(const clang::SourceManager &sourceManager,
                                       clang::CXXMethodDecl *method,
                                       clang::DeclContext *currentScope,
                                       clang::SourceLocation usageLoc, bool honourUsingDirectives);

/**
 * Returns true, if in a specific context, we can take the address of a method
 * for example doing &ClassName::SomePrivateMember might not be possible if the member is private
 * but might be possible if we're in a context which is friend of ClassName
 * Or it might be protected but context is a derived class
 *
 * When inside a derived class scope it's possible to take the address of a protected base method
 * but only if you qualify it with the derived class name, so &Derived::baseMethod, instead of &Base::baseMethod
 * If this was the case then isSpecialProtectedCase will be true
 */
CLAZYLIB_EXPORT bool canTakeAddressOf(clang::CXXMethodDecl *method, clang::DeclContext *context, bool &isSpecialProtectedCase);

}

#endif
