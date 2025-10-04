/*
    SPDX-FileCopyrightText: 2016-2017 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLAZY_TYPE_UTILS_H
#define CLAZY_TYPE_UTILS_H

#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/Expr.h>
#include <clang/AST/StmtCXX.h>
#include <clang/AST/Type.h>
#include <llvm/Support/Casting.h>

#include <string>
#include <vector>

namespace clang
{
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

class ClazyContext;

namespace clazy
{
/**
 * Returns the sizeof(void*) for the platform we're compiling for, in bits.
 */
inline int sizeOfPointer(const clang::ASTContext *context, const clang::QualType qt)
{
    if (!qt.getTypePtrOrNull()) {
        return -1;
    }
    // HACK: What's a better way of getting the size of a pointer ?
    return context->getTypeSize(context->getPointerType(qt));
}

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
bool classifyQualType(const ClazyContext *context,
                      clang::QualType qualType,
                      const clang::VarDecl *varDecl,
                      QualTypeClassification &classification,
                      clang::Stmt *body = nullptr);

/**
 * @brief Lighter version of classifyQualType in case you just want to know if it's small and trivially copyable&destructible
 * @param context The clazy context
 * @param qualType The QualType we're testing.
 * @return true if the type specified by QualType (or its pointee) are small and trivially copyable/destructible.
 */
bool isSmallTrivial(const ClazyContext *context, clang::QualType qualType);

/**
 * If qt is a reference, return it without a reference.
 * If qt is not a reference, return qt.
 *
 * This is useful because sometimes you have an argument like "const QString &", but qualType.isConstQualified()
 * returns false. Must go through qualType->getPointeeType().isConstQualified().
 */
inline clang::QualType unrefQualType(const clang::QualType qualType)
{
    const clang::Type *t = qualType.getTypePtrOrNull();
    return (t && t->isReferenceType()) ? t->getPointeeType() : qualType;
}

/**
 * If qt is a pointer or ref, return it without * or &.
 * Otherwise return qt unchanged.
 */
inline clang::QualType pointeeQualType(const clang::QualType qualType)
{
    // TODO: Make this recursive when we need to remove more than one level of *
    const clang::Type *t = qualType.getTypePtrOrNull();
    return (t && (t->isReferenceType() || t->isPointerType())) ? t->getPointeeType() : qualType;
}

/**
 * Returns if @p arg is stack or heap allocated.
 * true means it is. false means it either isn't or the situation was too complex to judge.
 * type is the qualified type
 */
void heapOrStackAllocated(clang::Expr *arg, const std::string &type, const clang::LangOptions &lo, bool &isStack, bool &isHeap);

/**
 * Returns true if t is an AutoType that can't be deduced.
 */
inline bool isUndeducibleAuto(const clang::Type *t)
{
    if (!t) {
        return false;
    }

    const auto *at = llvm::dyn_cast<clang::AutoType>(t);
    return at && at->getDeducedType().isNull();
}

inline const clang::Type *unpealAuto(clang::QualType q)
{
    if (q.isNull()) {
        return nullptr;
    }

    if (const auto *t = llvm::dyn_cast<clang::AutoType>(q.getTypePtr())) {
        return t->getDeducedType().getTypePtrOrNull();
    }

    return q.getTypePtr();
}

/**
 * Returns true if childDecl is a descent from parentDecl
 **/
bool derivesFrom(const clang::CXXRecordDecl *derived, const clang::CXXRecordDecl *possibleBase, std::vector<clang::CXXRecordDecl *> *baseClasses = nullptr);

// Overload
bool derivesFrom(const clang::CXXRecordDecl *derived, const std::string &possibleBase);

// Overload
bool derivesFrom(clang::QualType derived, const std::string &possibleBase);

/**
 * Returns the CXXRecordDecl represented by the CXXBaseSpecifier
 */
inline clang::CXXRecordDecl *recordFromBaseSpecifier(const clang::CXXBaseSpecifier &base)
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
inline bool valueIsConst(const clang::QualType qt)
{
    return pointeeQualType(qt).isConstQualified();
}

inline clang::CXXRecordDecl *typeAsRecord(const clang::QualType qt)
{
    if (qt.isNull()) {
        return nullptr;
    }

    return qt->getAsCXXRecordDecl();
}

inline clang::CXXRecordDecl *typeAsRecord(const clang::Expr *expr)
{
    if (!expr) {
        return nullptr;
    }

    return typeAsRecord(pointeeQualType(expr->getType()));
}

inline clang::CXXRecordDecl *typeAsRecord(const clang::ValueDecl *value)
{
    if (!value) {
        return nullptr;
    }

    return typeAsRecord(pointeeQualType(value->getType()));
}

/**
 * Returns the class that the typedef referred by qt is in.
 *
 * class Foo {
 *     typedef A B;
 * };
 *
 * For the above example Foo would be returned.
 */
inline clang::CXXRecordDecl *parentRecordForTypedef(const clang::QualType qt)
{
    auto *t = qt.getTypePtrOrNull();

    if (const auto *elab = llvm::dyn_cast<clang::ElaboratedType>(t)) {
        t = elab->desugar().getTypePtrOrNull();
    }

    if (const auto *tdt = llvm::dyn_cast<clang::TypedefType>(t)) {
        clang::TypedefNameDecl *tdnd = tdt->getDecl();
        return llvm::dyn_cast_or_null<clang::CXXRecordDecl>(tdnd->getDeclContext());
    }

    return nullptr;
}

// Example: const QString &
inline bool isConstRef(const clang::Type *t)
{
    return t && t->isReferenceType() && t->getPointeeType().isConstQualified();
}
}

#endif
