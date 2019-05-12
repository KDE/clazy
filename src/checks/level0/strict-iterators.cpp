/*
  This file is part of the clazy static checker.

    Copyright (C) 2017 Sergio Martins <smartins@kde.org>

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

#include "strict-iterators.h"
#include "ClazyContext.h"
#include "QtUtils.h"
#include "StringUtils.h"
#include "TypeUtils.h"
#include "SourceCompatibilityHelpers.h"
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

#include <assert.h>

using namespace clang;
using namespace std;

static bool isMemberVariable(Expr *expr)
{
    if (isa<MemberExpr>(expr))
        return true;

    if (auto ice = dyn_cast<ImplicitCastExpr>(expr))
        return isMemberVariable(ice->getSubExpr());

    return false;
}

// This got a bit messy since each Qt container produces a different AST, for example
// QVector::iterator isn't even a class, it's a typedef.

StrictIterators::StrictIterators(const std::string &name, ClazyContext *context)
    : CheckBase(name, context, Option_CanIgnoreIncludes)
{
}


void StrictIterators::VisitStmt(clang::Stmt *stmt)
{
    if (handleOperator(dyn_cast<CXXOperatorCallExpr>(stmt)))
        return;

    // QVector's aren't actual classes, they are just typedefs to T* and const T*
    handleImplicitCast(dyn_cast<ImplicitCastExpr>(stmt));
}

bool StrictIterators::handleImplicitCast(ImplicitCastExpr *implicitCast)
{
    if (!implicitCast)
        return false;

    const string nameTo = clazy::simpleTypeName(implicitCast->getType(), m_context->ci.getLangOpts());

    const QualType typeTo = implicitCast->getType();
    CXXRecordDecl *recordTo = clazy::parentRecordForTypedef(typeTo);
    if (recordTo && !clazy::isQtCOWIterableClass(recordTo))
        return false;

    recordTo = clazy::typeAsRecord(typeTo);
    if (recordTo && !clazy::isQtCOWIterator(recordTo))
        return false;

    assert(implicitCast->getSubExpr());

    if (isMemberVariable(implicitCast->getSubExpr())) {
        // Comparing a const_iterator against a member QVector<T>::iterator won't detach the container
        return false;
    }

    QualType typeFrom = implicitCast->getSubExpr()->getType();
    CXXRecordDecl *recordFrom = clazy::parentRecordForTypedef(typeFrom);
    if (recordFrom && !clazy::isQtCOWIterableClass(recordFrom))
        return false;

    // const_iterator might be a typedef to pointer, like const T *, instead of a class, so just check for const qualification in that case
    if (!(clazy::pointeeQualType(typeTo).isConstQualified() || clazy::endsWith(nameTo, "const_iterator")))
        return false;

    if (implicitCast->getCastKind() == CK_ConstructorConversion) {
        emitWarning(implicitCast, "Mixing iterators with const_iterators");
        return true;
    }

    // TODO: some util function to get the name of a nested class
    const bool nameToIsIterator = nameTo == "iterator" || clazy::endsWith(nameTo, "::iterator");
    if (nameToIsIterator)
        return false;

    const string nameFrom = clazy::simpleTypeName(typeFrom, m_context->ci.getLangOpts());
    const bool nameFromIsIterator = nameFrom == "iterator" || clazy::endsWith(nameFrom, "::iterator");
    if (!nameFromIsIterator)
        return false;

    auto p = m_context->parentMap->getParent(implicitCast);
    if (dyn_cast<CXXOperatorCallExpr>(p))
        return false;


    emitWarning(implicitCast, "Mixing iterators with const_iterators");

    return true;
}

bool StrictIterators::handleOperator(CXXOperatorCallExpr *op)
{
    if (!op)
        return false;

    auto method = dyn_cast_or_null<CXXMethodDecl>(op->getDirectCallee());
    if (!method || method->getNumParams() != 1)
        return false;

    CXXRecordDecl *record = method->getParent();
    if (!clazy::isQtCOWIterator(record))
        return false;

    if (clazy::name(record) != "iterator")
        return false;

    ParmVarDecl *p = method->getParamDecl(0);
    CXXRecordDecl *paramClass = p ? clazy::typeAsRecord(clazy::pointeeQualType(p->getType())) : nullptr;
    if (!paramClass || clazy::name(paramClass) != "const_iterator")
        return false;

    emitWarning(op, "Mixing iterators with const_iterators");
    return true;
}
