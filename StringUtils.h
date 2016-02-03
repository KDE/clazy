/*
   This file is part of the clazy static checker.

  Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
  Author: Sérgio Martins <sergio.martins@kdab.com>

  Copyright (C) 2015 Sergio Martins <smartins@kde.org>

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

#ifndef CLANG_LAZY_STRING_UTILS_H
#define CLANG_LAZY_STRING_UTILS_H

#include "checkmanager.h"
#include "Utils.h"
#include "HierarchyUtils.h"
#include "clazy_stl.h"

#include "clang/AST/PrettyPrinter.h"
#include <clang/Basic/LangOptions.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/DeclCXX.h>
#include <string>
#include <vector>

namespace clang {
class LangOpts;
}


namespace StringUtils {

template <typename T>
inline std::string classNameFor(T *ctorDecl)
{
    return std::string();
}

template <>
inline std::string classNameFor(clang::CXXConstructorDecl *ctorDecl)
{
    return ctorDecl->getParent()->getNameAsString();
}

template <>
inline std::string classNameFor(clang::CXXMethodDecl *method)
{
    return method->getParent()->getNameAsString();
}

template <>
inline std::string classNameFor(clang::CXXConstructExpr *expr)
{
    return classNameFor(expr->getConstructor());
}

template <>
inline std::string classNameFor(clang::ParmVarDecl *param)
{
    if (!param)
        return {};

    clang::QualType qt = param->getType();
    const clang::Type *t = qt.getTypePtrOrNull();
    if (!t)
        return {};

    const clang::CXXRecordDecl *record = t->isRecordType() ? t->getAsCXXRecordDecl()
                                                           : t->getPointeeCXXRecordDecl();

    if (record)
        return record->getNameAsString();

    return {};
}

template <typename T>
inline bool isOfClass(T *node, const std::string &className)
{
    return node && classNameFor<T>(node) == className;
}

inline bool functionIsOneOf(clang::FunctionDecl *func, const std::vector<std::string> &functionNames)
{
    return func && clazy_std::contains(functionNames, func->getNameAsString());
}

inline bool classIsOneOf(clang::CXXRecordDecl *record, const std::vector<std::string> &classNames)
{
    return record && clazy_std::contains(classNames, record->getNameAsString());
}

inline void printLocation(const clang::SourceManager &sm, const clang::SourceLocation &loc, bool newLine = true)
{
    llvm::errs() << loc.printToString(sm);
    if (newLine)
        llvm::errs() << "\n";
}

inline void printRange(const clang::SourceManager &sm, const clang::SourceRange &range, bool newLine = true)
{
    printLocation(sm, range.getBegin(), false);
    llvm::errs() << '-';
    printLocation(sm, range.getEnd(), newLine);
}

inline void printLocation(const clang::SourceManager &sm, const clang::Stmt *s, bool newLine = true)
{
    if (s)
        printLocation(sm, s->getLocStart(), newLine);
}

inline void printLocation(clang::PresumedLoc loc, bool newLine = true)
{
    llvm::errs() << loc.getFilename() << ' ' << loc.getLine() << ':' << loc.getColumn();
    if (newLine)
        llvm::errs() << "\n";
}

inline std::string qualifiedMethodName(clang::FunctionDecl *func)
{
    if (!func)
        return {};

    clang::CXXMethodDecl *method = clang::dyn_cast<clang::CXXMethodDecl>(func);
    if (!method)
        return func->getQualifiedNameAsString();

    // method->getQualifiedNameAsString() returns the name with template arguments, so do a little hack here
    if (!method || !method->getParent())
        return "";

    return method->getParent()->getNameAsString() + "::" + method->getNameAsString();
}

inline std::string qualifiedMethodName(clang::CallExpr *call)
{
    return call ? qualifiedMethodName(call->getDirectCallee()) : std::string();
}

inline void printParents(clang::ParentMap *map, clang::Stmt *s)
{
    int level = 0;
    llvm::errs() << (s ? s->getStmtClassName() : nullptr) << "\n";

    while (clang::Stmt *parent = HierarchyUtils::parent(map, s)) {
        ++level;
        llvm::errs() << std::string(level, ' ') << parent->getStmtClassName() << "\n";
        s = parent;
    }
}

inline std::string accessString(clang::AccessSpecifier s)
{
    switch (s)
    {
    case clang::AccessSpecifier::AS_public:
        return "public";
    case clang::AccessSpecifier::AS_private:
        return "private";
    case clang::AccessSpecifier::AS_protected:
        return "protected";
    case clang::AccessSpecifier::AS_none:
        return {};
    }
    return {};
}

/**
 * Returns the type of qt without CV qualifiers or references.
 * "const QString &" -> "QString"
 */
inline std::string simpleTypeName(clang::QualType qt, clang::LangOptions lo)
{
    return qt.getNonReferenceType().getUnqualifiedType().getAsString(clang::PrintingPolicy(lo));
}

inline std::string simpleTypeName(clang::ParmVarDecl *p, clang::LangOptions lo)
{
    return p ? simpleTypeName(p->getType(), lo) : std::string();
}

/**
 * Returns the type of an argument at index index without CV qualifiers or references.
 * void foo(int a, const QString &);
 * simpleArgTypeName(foo, 1, lo) would return "QString"
 */
std::string simpleArgTypeName(clang::FunctionDecl *func, uint index, clang::LangOptions);

/**
 * Returns true if any of the function's arguments if of type simpleType
 * By "simple" we mean without const or &, *
 */
bool anyArgIsOfSimpleType(clang::FunctionDecl *func, const std::string &simpleType, clang::LangOptions);

/**
 * Returns true if any of the function's arguments if of any of the types in simpleTypes
 * By "simple" we mean without const or &, *
 */
bool anyArgIsOfAnySimpleType(clang::FunctionDecl *func, const std::vector<std::string> &simpleTypes, clang::LangOptions);

inline void dump(const clang::SourceManager &sm, clang::Stmt *s)
{
    if (!s)
        return;

    llvm::errs() << "Start=" << s->getLocStart().printToString(sm)
                 << "; end=" << s->getLocStart().printToString(sm)
                 << "\n";

    for (auto child : s->children())
        dump(sm, child);
}

}

#endif

