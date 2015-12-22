/*
   This file is part of the clang-lazy static checker.

  Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
  Author: Sérgio Martins <sergio.martins@kdab.com>

  Copyright (C) 2015 Sergio Martins <smartins@kde.org>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

  As a special exception, permission is given to link this program
  with any edition of Qt, and distribute the resulting executable,
  without including the source code for Qt in the source distribution.
*/

#ifndef MOREWARNINGS_UTILS_H
#define MOREWARNINGS_UTILS_H

#include <clang/AST/DeclCXX.h>
#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/DeclTemplate.h>

#include <string>
#include <vector>
#include <map>

namespace clang {
    class CXXNamedCastExpr;
    class CXXRecordDecl;
    class CXXMemberCallExpr;
    class CXXConstructExpr;
    class CompilerInstance;
    class ClassTemplateSpecializationDecl;
    class Decl;
    class ParentMap;
    class SourceManager;
    class Stmt;
    class SourceLocation;
    class ExprWithCleanups;
    class ValueDecl;
    class ConditionalOperator;
    class CXXMethodDecl;
    class BinaryOperator;
}

namespace Utils {
    /// Returns true if the class inherits from QObject
    bool isQObject(clang::CXXRecordDecl *decl);

    /// Returns true if fullString ends in ending
    bool hasEnding(const std::string &fullString, const std::string &ending);

    /// Returns true if childDecl is a descent from parentDecl
    bool isChildOf(clang::CXXRecordDecl *childDecl, clang::CXXRecordDecl *parentDecl);

    bool isChildOf(clang::Stmt *child, clang::Stmt *parent);

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

    // Returns true of this value decl is a member variable of a class or struct
    // returns null if not
    clang::CXXRecordDecl* isMemberVariable(clang::ValueDecl *);

    /// Recursively goes through stmt's children and returns true if it finds a "break", "continue" or a "return" stmt
    /// All child statements that are on a source code line <
    /// If onlyBeforThisLoc is valid, then this function will only return true if the break/return/continue happens before
    bool loopCanBeInterrupted(clang::Stmt *loop, const clang::CompilerInstance &ci,
                              const clang::SourceLocation &onlyBeforeThisLoc);

    // Returns true if the class derived is or descends from a class named parent
    bool descendsFrom(clang::CXXRecordDecl *derived, const std::string &parentName);

    std::string qualifiedNameForDeclarationOfMemberExr(clang::MemberExpr *memberExpr);

    // Returns true if a body of statements contains a non const member call on object declared by varDecl
    // For example:
    // Foo foo; // this is the varDecl
    // while (bar) { foo.setValue(); // non-const call }
    bool containsNonConstMemberCall(clang::Stmt *body, const clang::VarDecl *varDecl);

    // Returns true if there's an assignment to varDecl in body
    bool containsAssignment(clang::Stmt *body, const clang::VarDecl *varDecl);

    // Returns true if a body of statements contains a function call that takes our variable (varDecl)
    // By ref or pointer
    bool containsCallByRef(clang::Stmt *body, const clang::VarDecl *varDecl);

    std::vector<std::string> splitString(const char *str, char separator);
    std::vector<std::string> splitString(const std::string &str, char separator);

    // QString::fromLatin1("foo")    -> true
    // QString::fromLatin1("foo", 1) -> false
    bool callHasDefaultArguments(clang::CallExpr *expr);

    // If there's a child of type StringLiteral, returns true
    // if allowEmpty is false, "" will be ignored
    bool containsStringLiteral(clang::Stmt *, bool allowEmpty = true, int depth = -1);

    bool isInsideOperatorCall(clang::ParentMap *map, clang::Stmt *s, const std::vector<std::string> &anyOf);
    bool insideCTORCall(clang::ParentMap *map, clang::Stmt *s, const std::vector<std::string> &anyOf);

    // returns true if the ternary operator has two string literal arguments, such as:
    // foo ? "bar" : "baz"
    bool ternaryOperatorIsOfStringLiteral(clang::ConditionalOperator*);

    bool isAssignOperator(clang::CXXOperatorCallExpr *op, const std::string &className, const std::string &argumentType);

    bool isImplicitCastTo(clang::Stmt *, const std::string &);

    clang::ClassTemplateSpecializationDecl *templateDecl(clang::Decl *decl);


    bool presumedLocationsEqual(const clang::PresumedLoc &l1, const clang::PresumedLoc &l2);

    // Returns the body of a for, while or do loop
    clang::Stmt *bodyFromLoop(clang::Stmt*);

    // Returns the list of methods with name methodName that the class/struct record contains
    std::vector<clang::CXXMethodDecl*> methodsFromString(const clang::CXXRecordDecl *record, const std::string &methodName);

    // Returns the most derived class. (CXXMemberCallExpr::getRecordDecl() return the first base class with the method)
    // The returned callee is the name of the variable on which the member call was made:
    // o1->foo() => "o1"
    // foo() => "this"
    const clang::CXXRecordDecl* recordForMemberCall(clang::CXXMemberCallExpr *call, std::string &implicitCallee);


    // The list of namespaces, classes, inner classes. The inner ones are at the beginning of the list
    std::vector<clang::DeclContext *> contextsForDecl(clang::DeclContext *);

    clang::CXXRecordDecl *firstMethodOrClassContext(clang::DeclContext *);

    // Returns fully/smi-fully qualified name for a method
    // but doesn't overqualify with namespaces which we're already in.
    // if currentScope == nullptr will return a fully qualified name
    std::string getMostNeededQualifiedName(const clang::SourceManager &sourceManager, clang::CXXMethodDecl *method, clang::DeclContext *currentScope, clang::SourceLocation usageLoc, bool honourUsingDirectives);

    // Returns true, if in a specific context, we can take the address of a method
    // for example doing &ClassName::SomePrivateMember might not be possible if the member is private
    // but might be possible if we're in a context which is friend of ClassName
    // Or it might be protected but context is a derived class
    //
    // When inside a derived class scope it's possible to take the address of a protected base method
    // but only if you qualify it with the derived class name, so &Derived::baseMethod, instead of &Base::baseMethod
    // If this was the case then isSpecialProtectedCase will be true

    bool canTakeAddressOf(clang::CXXMethodDecl *method, clang::DeclContext *context, bool &isSpecialProtectedCase);

    // Convertible means that a signal with of type source can connect to a signal/slot of type target
    bool isConvertibleTo(const clang::Type *source, const clang::Type *target);

    bool isAscii(clang::StringLiteral *lt);

    // Checks if Statement s inside an operator* call
    bool isInDerefExpression(clang::Stmt *s, clang::ParentMap *map);

    // For a a chain called expression like foo().bar().baz() returns a list of calls
    // {baz(), bar(), foo()}

    // the parameter lastCallExpr to pass would be baz()
    // No need to specify the other callexprs, they are children of the first one, since the AST looks like:
    // - baz
    // -- bar
    // --- foo
    std::vector<clang::CallExpr *> callListForChain(clang::CallExpr *lastCallExpr);

    // Returns the first base class
    clang::CXXRecordDecl * rootBaseClass(clang::CXXRecordDecl *derived);

    // Returns the copy ctor for this class
    clang::CXXConstructorDecl *copyCtor(clang::CXXRecordDecl *);

    // Returns the copy-assignment operator for this class
    clang::CXXMethodDecl *copyAssign(clang::CXXRecordDecl *);

    bool hasMember(clang::CXXRecordDecl *record, const std::string &memberTypeName);

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
    bool classifyQualType(const clang::CompilerInstance &ci, const clang::VarDecl *varDecl,
                          QualTypeClassification &classification,
                          clang::Stmt *body = nullptr);


    /**
     * If qt is a reference, return it without a reference.
     * If qt is nor a reference, return qt.
     *
     * This is useful because sometimes you have an argument like "const QString &", but qualType.isConstQualified()
     * returns false. Must go through qualType->getPointeeType().isConstQualified().
     */
    clang::QualType unrefQualType(const clang::QualType &qt);

    /**
     * Returns true if record is a shared pointer (boost, Qt or stl only).
     */
    bool isSharedPointer(clang::CXXRecordDecl *record);


    /**
     * Returns true if the source location loc is inside a macro named macroName.
     */
    bool isInMacro(const clang::CompilerInstance &ci, clang::SourceLocation loc, const std::string &macroName);
}

#endif
