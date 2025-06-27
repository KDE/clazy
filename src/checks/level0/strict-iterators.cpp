/*
    SPDX-FileCopyrightText: 2017 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "strict-iterators.h"
#include "ClazyContext.h"
#include "QtUtils.h"
#include "StringUtils.h"
#include "TypeUtils.h"
#include "clazy_stl.h"

#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/OperationKinds.h>
#include <clang/AST/ParentMap.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/Type.h>
#include <clang/Basic/LLVM.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Frontend/CompilerInstance.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Casting.h>

#include <cassert>

using namespace clang;

static bool isMemberVariable(Expr *expr)
{
    if (isa<MemberExpr>(expr)) {
        return true;
    }

    if (auto *ice = dyn_cast<ImplicitCastExpr>(expr)) {
        return isMemberVariable(ice->getSubExpr());
    }

    return false;
}

// This got a bit messy since each Qt container produces a different AST, for example
// QVector::iterator isn't even a class, it's a typedef.

StrictIterators::StrictIterators(const std::string &name)
    : CheckBase(name, Option_CanIgnoreIncludes)
{
}

void StrictIterators::VisitStmt(clang::Stmt *stmt)
{
    if (handleOperator(dyn_cast<CXXOperatorCallExpr>(stmt))) {
        return;
    }

    // QVector's aren't actual classes, they are just typedefs to T* and const T*
    handleImplicitCast(dyn_cast<ImplicitCastExpr>(stmt));
}

bool StrictIterators::handleImplicitCast(ImplicitCastExpr *implicitCast)
{
    if (!implicitCast) {
        return false;
    }

    const std::string nameTo = clazy::simpleTypeName(implicitCast->getType(), lo());

    const QualType typeTo = implicitCast->getType();
    CXXRecordDecl *recordTo = clazy::parentRecordForTypedef(typeTo);
    if (recordTo && !clazy::isQtCOWIterableClass(recordTo)) {
        return false;
    }

    recordTo = clazy::typeAsRecord(typeTo);
    if (recordTo && !clazy::isQtCOWIterator(recordTo)) {
        return false;
    }

    assert(implicitCast->getSubExpr());

    if (isMemberVariable(implicitCast->getSubExpr())) {
        // Comparing a const_iterator against a member QVector<T>::iterator won't detach the container
        return false;
    }

    QualType typeFrom = implicitCast->getSubExpr()->getType();
    CXXRecordDecl *recordFrom = clazy::parentRecordForTypedef(typeFrom);
    if (recordFrom && !clazy::isQtCOWIterableClass(recordFrom)) {
        return false;
    }

    // const_iterator might be a typedef to pointer, like const T *, instead of a class, so just check for const qualification in that case
    if (!(clazy::pointeeQualType(typeTo).isConstQualified() || clazy::endsWith(nameTo, "const_iterator"))) {
        return false;
    }

    // Allow conversions for mutating member functions of Qt container classes
    if (implicitCast->getCastKind() == CK_ConstructorConversion) {
        if (auto *memberCall = dyn_cast_or_null<CXXMemberCallExpr>(m_context->parentMap->getParent(implicitCast))) {
            auto memberFunctionDecl = memberCall->getMethodDecl();
            if (auto *parentClass = memberFunctionDecl->getParent()) {
                static const std::vector<std::string> allow = {
                    "QMap<>::insert",
                    "QMap<>::erase",
                    "QHash<>::erase",
                    "QMultiHash<>::erase",
                    "QList<>::emplace",
                    "QList<>::erase",
                    "QList<>::insert",
                    "QVarLengthArray<>::emplace",
                    "QVarLengthArray<>::erase",
                    "QVarLengthArray<>::insert",
                    "QSet<>::erase",
                    "QSet<>::insert",
                    "QMultiMap<>::erase",
                    "QMultiMap<>::insert",
                };

                const auto qualifiedName = parentClass->getNameAsString() + "<>::" + memberFunctionDecl->getNameAsString();
                if (clazy::contains(allow, qualifiedName)) {
                    return false;
                }
            }
        }

        emitWarning(implicitCast, "Mixing iterators with const_iterators");
        return true;
    }

    // TODO: some util function to get the name of a nested class
    const bool nameToIsIterator = nameTo == "iterator" || clazy::endsWith(nameTo, "::iterator");
    if (nameToIsIterator) {
        return false;
    }

    const std::string nameFrom = clazy::simpleTypeName(typeFrom, lo());
    const bool nameFromIsIterator = nameFrom == "iterator" || clazy::endsWith(nameFrom, "::iterator");
    if (!nameFromIsIterator) {
        return false;
    }

    auto *p = m_context->parentMap->getParent(implicitCast);
    if (isa<CXXOperatorCallExpr>(p)) {
        return false;
    }

    emitWarning(implicitCast, "Mixing iterators with const_iterators");

    return true;
}

bool StrictIterators::handleOperator(CXXOperatorCallExpr *op)
{
    if (!op) {
        return false;
    }

    auto *method = dyn_cast_or_null<CXXMethodDecl>(op->getDirectCallee());
    if (!method || method->getNumParams() != 1) {
        return false;
    }

    CXXRecordDecl *record = method->getParent();
    if (!clazy::isQtCOWIterator(record)) {
        return false;
    }

    if (clazy::name(record) != "iterator") {
        return false;
    }

    ParmVarDecl *p = method->getParamDecl(0);
    const CXXRecordDecl *paramClass = p ? clazy::typeAsRecord(clazy::pointeeQualType(p->getType())) : nullptr;
    if (!paramClass || clazy::name(paramClass) != "const_iterator") {
        return false;
    }

    emitWarning(op, "Mixing iterators with const_iterators");
    return true;
}
