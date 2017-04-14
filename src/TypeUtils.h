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

#ifndef CLAZY_TYPE_UTILS_H
#define CLAZY_TYPE_UTILS_H

#include "clazylib_export.h"

#include <clang/AST/Type.h>
#include <clang/AST/Expr.h>
#include <clang/AST/StmtCXX.h>
#include <clang/AST/DeclCXX.h>
#include <string>

namespace clang {
class CompilerInstance;
class Expr;
class LangOptions;
class QualType;
class Stmt;
class VarDecl;
class Type;
class CXXRecordDecl;
class CXXBaseSpecifier;
}

namespace TypeUtils
{
    /**
     * Returns the sizeof(void*) for the platform we're compiling for, in bits.
     */
    CLAZYLIB_EXPORT int sizeOfPointer(const clang::CompilerInstance &, clang::QualType qt);

    struct QualTypeClassification {
        bool isConst = false;
        bool isReference = false;
        bool isBig = false;
        bool isNonTriviallyCopyable = false;
        bool passBigTypeByConstRef = false;
        bool passNonTriviallyCopyableByConstRef = false;
        bool passSmallTrivialByValue = false;
        int size_of_T = 0;
    };

    /**
     * Classifies a QualType, for example:
     *
     * This function is useful to know if a type should be passed by value or const-ref.
     * The optional parameter body is in order to advise non-const-ref -> value, since the body
     * needs to be inspected to see if we that would compile.
     */
    CLAZYLIB_EXPORT bool classifyQualType(const clang::CompilerInstance &ci, const clang::VarDecl *varDecl,
                          QualTypeClassification &classification,
                          clang::Stmt *body = nullptr);

    /**
     * If qt is a reference, return it without a reference.
     * If qt is not a reference, return qt.
     *
     * This is useful because sometimes you have an argument like "const QString &", but qualType.isConstQualified()
     * returns false. Must go through qualType->getPointeeType().isConstQualified().
     */
    inline clang::QualType unrefQualType(clang::QualType qualType)
    {
        const clang::Type *t = qualType.getTypePtrOrNull();
        return (t && t->isReferenceType()) ? t->getPointeeType() : qualType;
    }

    /**
     * If qt is a pointer or ref, return it without * or &.
     * Otherwise return qt unchanged.
     */
    inline clang::QualType pointeeQualType(clang::QualType qualType)
    {
        // TODO: Make this recursive when we need to remove more than one level of *
        const clang::Type *t = qualType.getTypePtrOrNull();
        return (t && (t->isReferenceType() || t->isPointerType())) ? t->getPointeeType() : qualType;
    }

    /**
     * Returns if @p arg is stack or heap allocated.
     * true means it is. false means it either isn't or the situation was too complex to judge.
     */
    CLAZYLIB_EXPORT void heapOrStackAllocated(clang::Expr *arg, const std::string &type,
                                              const clang::LangOptions &lo,
                                              bool &isStack, bool &isHeap);

    /**
     * Returns true if t is an AutoType that can't be deduced.
     */
    inline bool isUndeducibleAuto(const clang::Type *t)
    {
        if (!t)
            return false;

        auto at = llvm::dyn_cast<clang::AutoType>(t);
        return at && at->getDeducedType().isNull();
    }

    /**
     * Returns true if childDecl is a descent from parentDecl
     **/
    CLAZYLIB_EXPORT bool derivesFrom(clang::CXXRecordDecl *derived, clang::CXXRecordDecl *possibleBase);

    // Overload
    CLAZYLIB_EXPORT bool derivesFrom(clang::CXXRecordDecl *derived, const std::string &possibleBase);

    // Overload
    CLAZYLIB_EXPORT bool derivesFrom(clang::QualType derived, const std::string &possibleBase);

    /**
     * Returns the CXXRecordDecl represented by the CXXBaseSpecifier
     */
    inline clang::CXXRecordDecl * recordFromBaseSpecifier(const clang::CXXBaseSpecifier &base)
    {
        const clang::Type *t = base.getType().getTypePtrOrNull();
        return t ? t->getAsCXXRecordDecl() : nullptr;
    }
    /**
     * Returns true if the value is const. This is usually equivalent to qt.isConstQualified() but
     * takes care of the special case where qt represents a pointer. Many times you don't care if the
     * pointer is const or not and just care about the pointee.
     *
     * A a;        => false
     * const A a;  => true
     * A* a;       => false
     * const A* a; => true
     * A *const a; => false
     */
    inline bool valueIsConst(clang::QualType qt)
    {
        return pointeeQualType(qt).isConstQualified();
    }

    inline clang::CXXRecordDecl* typeAsRecord(clang::QualType qt)
    {
        if (qt.isNull())
            return nullptr;

        return qt->getAsCXXRecordDecl();
    }

    inline clang::CXXRecordDecl* typeAsRecord(clang::Expr *expr)
    {
        if (!expr)
            return nullptr;

        return typeAsRecord(pointeeQualType(expr->getType()));
    }
}

#endif
