/**********************************************************************
**  Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
**  Author: Sérgio Martins <sergio.martins@kdab.com>
**
** This file may be distributed and/or modified under the terms of the
** GNU Lesser General Public License version 2.1 and version 3 as published by the
** Free Software Foundation and appearing in the file LICENSE.LGPL.txt included.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**********************************************************************/

#ifndef MOREWARNINGS_UTILS_H
#define MOREWARNINGS_UTILS_H

#include <clang/AST/DeclCXX.h>
#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/DeclTemplate.h>

#include <string>
#include <vector>

namespace clang {
    class CXXNamedCastExpr;
    class CXXRecordDecl;
    class CXXMemberCallExpr;
    class CXXConstructExpr;
    class CompilerInstance;
    class ClassTemplateSpecializationDecl;
    class Decl;
    class ParentMap;
    class Stmt;
    class SourceLocation;
    class ExprWithCleanups;
    class ValueDecl;
}

namespace Utils {
    /// Returns true if the class inherits from QObject
    bool isQObject(clang::CXXRecordDecl *decl);

    /// Prints a compiler warning to stderr
    void emitWarning(clang::CompilerInstance &instance, clang::SourceLocation loc, const char *error);

    /// Returns true if fullString ends in ending
    bool hasEnding(const std::string &fullString, const std::string &ending);

    /// Returns true if childDecl is a descent from parentDecl
    bool isChildOf(clang::CXXRecordDecl *childDecl, clang::CXXRecordDecl *parentDecl);

    /// Returns true if the class has at least one constexpr ctor
    bool hasConstexprCtor(clang::CXXRecordDecl *decl);

    /// Returns the type we're casting *from*
    clang::CXXRecordDecl * namedCastInnerDecl(clang::CXXNamedCastExpr *staticOrDynamicCast);

    /// Returns the type we're casting *to*
    clang::CXXRecordDecl * namedCastOuterDecl(clang::CXXNamedCastExpr *staticOrDynamicCast);

    /// Returns the class declaration from a variable declaration
    // So, if the var decl is "Foo f"; it returns the declaration of Foo
    clang::CXXRecordDecl * recordFromVarDecl(clang::Decl *);

    /// Returns the template specialization from a variable declaration
    // So, if the var decl is "QList<Foo> f;", returns the template specialization QList<Foo>
    clang::ClassTemplateSpecializationDecl * templateSpecializationFromVarDecl(clang::Decl *);

    /// Returns true if stmt is inside a function named name
    //bool statementIsInFunc(clang::ParentMap *, clang::Stmt *stmt, const std::string &name);

    /// Returns true if stm is parent of a member function call named "name"
    bool isParentOfMemberFunctionCall(clang::Stmt *stm, const std::string &name);

    /// Returns true if all the child member function calls are const functions.
    bool allChildrenMemberCallsConst(clang::Stmt *stm);

    /// Returns true if at least a UnaryOperator, BinaryOperator or non-const function call is found
    bool childsHaveSideEffects(clang::Stmt *stm);

    /// Receives a member call, such as "list.reserve()" and returns the declaration of the variable list
    // such as "QList<F> list"
    clang::ValueDecl * valueDeclForMemberCall(clang::CXXMemberCallExpr *);

    /// Receives an operator call, such as "list << fooo" and returns the declaration of the variable list
    // such as "QList<F> list"
    clang::ValueDecl * valueDeclForOperatorCall(clang::CXXOperatorCallExpr *);

    /// Returns true if a decl is inside a function, instead of say a class field.
    /// This returns true if "QList<int> l;" is a localvariable, instead of being a class field such
    /// as struct Foo { QList<int> l; }
    bool isValueDeclInFunctionContext(clang::ValueDecl *);

    /// Recursively goes through stmt's children and returns true if it finds a "break", "continue" or a "return" stmt
    /// All child statements that are on a source code line <
    /// If onlyBeforThisLoc is valid, then this function will only return true if the break/return/continue happens before
    bool loopCanBeInterrupted(clang::Stmt *loop, clang::CompilerInstance &ci, const clang::SourceLocation &onlyBeforeThisLoc);

    // Returns true if the class derived is or descends from a class named parent
    bool descendsFrom(clang::CXXRecordDecl *derived, const std::string &parentName);

    std::string qualifiedNameForDeclarationOfMemberExr(clang::MemberExpr *memberExpr);

    template <typename T>
    void getChilds2(clang::Stmt *stmt, std::vector<T*> &result_list)
    {
        if (stmt == nullptr)
            return;

        auto cexpr = llvm::dyn_cast<T>(stmt);
        if (cexpr != nullptr)
            result_list.push_back(cexpr);

        auto it = stmt->child_begin();
        auto end = stmt->child_end();

        for (; it != end; ++it) {
            getChilds2(*it, result_list);
        }
    }

    /// Goes into a statement and returns it's childs of type T
    /// It only goes down 1 level of children, except if there's a ExprWithCleanups, which we unpeal

    template <typename T>
    void getChilds(clang::Stmt *stm, std::vector<T*> &result_list)
    {
        if (stm == nullptr)
            return;

        auto expr = clang::dyn_cast<T>(stm);
        if (expr) {
            result_list.push_back(expr);
            return;
        }

        auto it = stm->child_begin();
        auto e = stm->child_end();
        for (; it != e; ++it) {
            if (*it == nullptr) // Can happen
                continue;

            auto expr = clang::dyn_cast<T>(*it);
            if (expr) {
                result_list.push_back(expr);
                continue;
            }

            auto cleanups = clang::dyn_cast<clang::ExprWithCleanups>(*it);
            if (cleanups) {
                getChilds<T>(cleanups, result_list);
            }
        }
    }

    clang::ClassTemplateSpecializationDecl *templateDecl(clang::Decl *decl);
}

#endif
