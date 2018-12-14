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

#include "Utils.h"
#include "clazy_stl.h"
#include "SourceCompatibilityHelpers.h"

#include <clang/Basic/LangOptions.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/PrettyPrinter.h>
#include <clang/AST/Decl.h>
#include <clang/AST/DeclarationName.h>
#include <clang/AST/Expr.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/Type.h>
#include <clang/Basic/LLVM.h>
#include <clang/Basic/OperatorKinds.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/Specifiers.h>
#include <llvm/ADT/ArrayRef.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/raw_ostream.h>

#include <string>
#include <vector>

namespace clang {
class LangOpts;
class SourceManager;
}


namespace clazy {

// Returns the class name.
// The name will not include any templates, so  "QVector::iterator" would be returned for QVector<int>::iterator
// Use record->getQualifiedNameAsString() if you want the templates.

inline std::string classNameFor(const clang::CXXRecordDecl *record)
{
    if (!record)
        return {};

    const std::string name = record->getNameAsString();

    if (auto p = record->getParent()) {
        // TODO: Also append the namespace, when needed.
        auto parentName = classNameFor(llvm::dyn_cast<clang::CXXRecordDecl>(p));
        if (!parentName.empty())
            return parentName + "::" + name;
    }

    return name;
}

inline std::string classNameFor(clang::CXXConstructorDecl *ctorDecl)
{
    return classNameFor(ctorDecl->getParent());
}

inline std::string classNameFor(clang::CXXMethodDecl *method)
{
    return method ? classNameFor(method->getParent()) : std::string();
}

inline std::string classNameFor(clang::CXXConstructExpr *expr)
{
    return classNameFor(expr->getConstructor());
}

inline std::string classNameFor(clang::CXXOperatorCallExpr *call)
{
    return call ? classNameFor(llvm::dyn_cast_or_null<clang::CXXMethodDecl>(call->getDirectCallee())) : std::string();
}

inline std::string classNameFor(clang::QualType qt)
{
    qt = qt.getNonReferenceType().getUnqualifiedType();
    const clang::Type *t = qt.getTypePtrOrNull();
    if (!t)
        return {};

    if (clang::ElaboratedType::classof(t))
        return classNameFor(static_cast<const clang::ElaboratedType*>(t)->getNamedType());

    const clang::CXXRecordDecl *record = t->isRecordType() ? t->getAsCXXRecordDecl()
                                                           : t->getPointeeCXXRecordDecl();
    return classNameFor(record);
}

inline std::string classNameFor(clang::ParmVarDecl *param)
{
    if (!param)
        return {};

    return classNameFor(param->getType());
}

inline llvm::StringRef name(const clang::NamedDecl *decl)
{
    if (decl->getDeclName().isIdentifier())
        return decl->getName();

    return "";
}

inline llvm::StringRef name(const clang::CXXMethodDecl *method)
{
    auto op = method->getOverloadedOperator();
    if (op == clang::OO_Subscript)
        return "operator[]";
    if (op == clang::OO_LessLess)
        return "operator<<";
    if (op == clang::OO_PlusEqual)
        return "operator+=";

    return name(static_cast<const clang::NamedDecl *>(method));
}

inline llvm::StringRef name(const clang::CXXConstructorDecl *decl)
{
    return name(decl->getParent());
}

inline llvm::StringRef name(const clang::CXXDestructorDecl *decl)
{
    return name(decl->getParent());
}

// Returns the type name with or without namespace, depending on how it was written by the user.
// If the user omitted the namespace then the return won't have namespace
inline std::string name(clang::QualType t, clang::LangOptions lo, bool asWritten)
{
    clang::PrintingPolicy p(lo);
    p.SuppressScope = asWritten;
    return t.getAsString(p);
}

template <typename T>
inline bool isOfClass(T *node, llvm::StringRef className)
{
    return node && classNameFor(node) == className;
}

inline bool functionIsOneOf(clang::FunctionDecl *func, const std::vector<llvm::StringRef> &functionNames)
{
    return func && clazy::contains(functionNames, clazy::name(func));
}

inline bool classIsOneOf(clang::CXXRecordDecl *record, const std::vector<llvm::StringRef> &classNames)
{
    return record && clazy::contains(classNames, clazy::name(record));
}

inline void printLocation(const clang::SourceManager &sm, clang::SourceLocation loc, bool newLine = true)
{
    llvm::errs() << loc.printToString(sm);
    if (newLine)
        llvm::errs() << "\n";
}

inline void printRange(const clang::SourceManager &sm, clang::SourceRange range, bool newLine = true)
{
    printLocation(sm, range.getBegin(), false);
    llvm::errs() << '-';
    printLocation(sm, range.getEnd(), newLine);
}

inline void printLocation(const clang::SourceManager &sm, const clang::Stmt *s, bool newLine = true)
{
    if (s)
        printLocation(sm, clazy::getLocStart(s), newLine);
}

inline void printLocation(const clang::PresumedLoc &loc, bool newLine = true)
{
    llvm::errs() << loc.getFilename() << ' ' << loc.getLine() << ':' << loc.getColumn();
    if (newLine)
        llvm::errs() << "\n";
}

inline std::string qualifiedMethodName(clang::FunctionDecl *func)
{
    if (!func)
        return {};

    auto method = clang::dyn_cast<clang::CXXMethodDecl>(func);
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
inline std::string simpleTypeName(clang::QualType qt, const clang::LangOptions &lo)
{
    auto t = qt.getTypePtrOrNull();
    if (!t)
        return {};

    if (clang::ElaboratedType::classof(t))
        qt = static_cast<const clang::ElaboratedType*>(t)->getNamedType();

    return qt.getNonReferenceType().getUnqualifiedType().getAsString(clang::PrintingPolicy(lo));
}

inline std::string simpleTypeName(clang::ParmVarDecl *p, const clang::LangOptions &lo)
{
    return p ? simpleTypeName(p->getType(), lo) : std::string();
}

inline std::string typeName(clang::QualType qt, const clang::LangOptions &lo, bool simpleName)
{
    return simpleName ? simpleTypeName(qt, lo) : qt.getAsString(lo);
}

/**
 * Returns the type name of the return type.
 * If \a simpleName is true, any cv qualifications, ref or pointer are not taken into account, so
 * const Foo & would be equal to Foo.
 */
inline std::string returnTypeName(clang::CallExpr *call, const clang::LangOptions &lo,
                                  bool simpleName = true)
{
    if (!call)
        return {};

    clang::FunctionDecl *func = call->getDirectCallee();
    return func ? clazy::typeName(func->getReturnType(), lo, simpleName) : std::string();
}

inline bool hasArgumentOfType(clang::FunctionDecl *func, llvm::StringRef typeName,
                              const clang::LangOptions &lo, bool simpleName = true)
{
    return clazy::any_of(Utils::functionParameters(func), [simpleName, lo, typeName](clang::ParmVarDecl *param) {
            return clazy::typeName(param->getType(), lo, simpleName) == typeName;
        });
}

/**
 * Returns the type of an argument at index index without CV qualifiers or references.
 * void foo(int a, const QString &);
 * simpleArgTypeName(foo, 1, lo) would return "QString"
 */
std::string simpleArgTypeName(clang::FunctionDecl *func, unsigned int index, const clang::LangOptions &);

/**
 * Returns true if any of the function's arguments if of type simpleType
 * By "simple" we mean without const or &, *
 */
bool anyArgIsOfSimpleType(clang::FunctionDecl *func, const std::string &simpleType, const clang::LangOptions &);

/**
 * Returns true if any of the function's arguments if of any of the types in simpleTypes
 * By "simple" we mean without const or &, *
 */
bool anyArgIsOfAnySimpleType(clang::FunctionDecl *func, const std::vector<std::string> &simpleTypes, const clang::LangOptions &);

inline void dump(const clang::SourceManager &sm, clang::Stmt *s)
{
    if (!s)
        return;

    llvm::errs() << "Start=" << getLocStart(s).printToString(sm)
                 << "; end=" << getLocStart(s).printToString(sm)
                 << "\n";

    for (auto child : s->children())
        dump(sm, child);
}

}

#endif

