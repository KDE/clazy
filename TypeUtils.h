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

namespace clang {
class CompilerInstance;
class QualType;
class Stmt;
class VarDecl;
class Type;
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
    CLAZYLIB_EXPORT clang::QualType unrefQualType(clang::QualType qt);
}

#endif
