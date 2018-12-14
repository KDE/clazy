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

#include <clang/AST/Type.h>

#include <vector>
#include <string>

namespace clang {
class CXXMethodDecl;
class CXXRecordDecl;
class ClassTemplateSpecializationDecl;
class Decl;
class LangOptions;
}

namespace clazy
{
/**
 * Returns a list of QualTypes for the template arguments.
 * For example:
 *    If the method was foo<int, Bar, char*>(), it would return {int, Bar, Char*}
 */
std::vector<clang::QualType> getTemplateArgumentsTypes(clang::CXXMethodDecl *);

/**
 * Returns a list of QualTypes for the template arguments.
 * For example:
 *    If the class was QList<int>(), it would return {int}
 */
std::vector<clang::QualType> getTemplateArgumentsTypes(clang::CXXRecordDecl *);

clang::ClassTemplateSpecializationDecl *templateDecl(clang::Decl *decl);

/**
 * Returns a string with the type name of the argument at the specified index.
 * If recordOnly is true, then it will only return a name if the argument is a class or struct.
 *
 * Example: For QList<Foo>, getTemplateArgumentTypeStr(decl, 0) would return "Foo"
 */
std::string getTemplateArgumentTypeStr(clang::ClassTemplateSpecializationDecl*,
                                       unsigned int index, const clang::LangOptions &lo, bool recordOnly = false);

clang::QualType getTemplateArgumentType(clang::ClassTemplateSpecializationDecl *, unsigned int index);

}
