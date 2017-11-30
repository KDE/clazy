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
#include "Utils.h"
#include "HierarchyUtils.h"
#include "QtUtils.h"
#include "StringUtils.h"
#include "TypeUtils.h"
#include "checkmanager.h"

#include <clang/AST/AST.h>

using namespace clang;
using namespace std;

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

    const string nameTo = StringUtils::simpleTypeName(implicitCast->getType(), m_context->ci.getLangOpts());

    const QualType typeTo = implicitCast->getType();
    CXXRecordDecl *recordTo = TypeUtils::parentRecordForTypedef(typeTo);
    if (recordTo && !QtUtils::isQtCOWIterableClass(recordTo))
        return false;

    recordTo = TypeUtils::typeAsRecord(typeTo);
    if (recordTo && !QtUtils::isQtCOWIterator(recordTo))
        return false;

    assert(implicitCast->getSubExpr());
    QualType typeFrom = implicitCast->getSubExpr()->getType();
    CXXRecordDecl *recordFrom = TypeUtils::parentRecordForTypedef(typeFrom);
    if (recordFrom && !QtUtils::isQtCOWIterableClass(recordFrom))
        return false;

    // const_iterator might be a typedef to pointer, like const T *, instead of a class, so just check for const qualification in that case
    if (!(TypeUtils::pointeeQualType(typeTo).isConstQualified() || clazy_std::endsWith(nameTo, "const_iterator")))
        return false;

    if (implicitCast->getCastKind() == CK_ConstructorConversion) {
        emitWarning(implicitCast, "Mixing iterators with const_iterators");
        return true;
    }

    // TODO: some util function to get the name of a nested class
    const  bool nameToIsIterator = nameTo == "iterator" || clazy_std::endsWith(nameTo, "::iterator");
    if (nameToIsIterator)
        return false;

    const string nameFrom = StringUtils::simpleTypeName(typeFrom, m_context->ci.getLangOpts());
    const  bool nameFromIsIterator = nameFrom == "iterator" || clazy_std::endsWith(nameFrom, "::iterator");
    if (!nameFromIsIterator)
        return false;

    if (recordTo && clazy_std::startsWith(recordTo->getQualifiedNameAsString(), "OrderedSet")) {
        string filename = m_sm.getFilename(implicitCast->getLocStart());
        if (filename == "lalr.cpp") // Lots of false positives here, because of const_iterator -> iterator typedefs
            return false;
    }

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
    if (!QtUtils::isQtCOWIterator(record))
        return false;

    if (record->getNameAsString() != "iterator")
        return false;

    ParmVarDecl *p = method->getParamDecl(0);
    CXXRecordDecl *paramClass = p ? TypeUtils::typeAsRecord(TypeUtils::pointeeQualType(p->getType())) : nullptr;
    if (!paramClass || paramClass->getNameAsString() != "const_iterator")
        return false;

    emitWarning(op, "Mixing iterators with const_iterators");
    return true;
}

REGISTER_CHECK("strict-iterators", StrictIterators, CheckLevel0)
