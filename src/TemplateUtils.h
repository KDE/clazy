/*
    This file is part of the clazy static checker.

    SPDX-FileCopyrightText: 2016 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <clang/AST/Type.h>

#include <string>
#include <vector>

namespace clang
{
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
std::string getTemplateArgumentTypeStr(clang::ClassTemplateSpecializationDecl *, unsigned int index, const clang::LangOptions &lo, bool recordOnly = false);

clang::QualType getTemplateArgumentType(clang::ClassTemplateSpecializationDecl *, unsigned int index);

}
