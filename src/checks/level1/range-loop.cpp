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

#include "range-loop.h"
#include "Utils.h"
#include "QtUtils.h"
#include "TypeUtils.h"
#include "StringUtils.h"
#include "LoopUtils.h"
#include "StmtBodyRange.h"

#include <clang/AST/AST.h>

using namespace clang;
using namespace std;

RangeLoop::RangeLoop(const std::string &name, ClazyContext *context)
    : CheckBase(name, context, Option_CanIgnoreIncludes)
{
}

void RangeLoop::VisitStmt(clang::Stmt *stmt)
{
    if (auto rangeLoop = dyn_cast<CXXForRangeStmt>(stmt)) {
        processForRangeLoop(rangeLoop);
    }
}

void RangeLoop::processForRangeLoop(CXXForRangeStmt *rangeLoop)
{
    Expr *containerExpr = rangeLoop->getRangeInit();
    if (!containerExpr)
        return;

    QualType qt = containerExpr->getType();
    const Type *t = qt.getTypePtrOrNull();
    if (!t || !t->isRecordType())
        return;

    checkPassByConstRefCorrectness(rangeLoop);

    if (qt.isConstQualified()) // const won't detach
        return;

    auto loopVariableType = rangeLoop->getLoopVariable()->getType();
    if (!TypeUtils::unrefQualType(loopVariableType).isConstQualified() && loopVariableType->isReferenceType())
        return;

    CXXRecordDecl *record = t->getAsCXXRecordDecl();
    if (!clazy::isQtCOWIterableClass(Utils::rootBaseClass(record)))
        return;

    StmtBodyRange bodyRange(nullptr, &sm(), rangeLoop->getLocStart());
    if (clazy::containerNeverDetaches(clazy::containerDeclForLoop(rangeLoop), bodyRange))
        return;

    emitWarning(rangeLoop->getLocStart(), "c++11 range-loop might detach Qt container (" + record->getQualifiedNameAsString() + ')');
}

void RangeLoop::checkPassByConstRefCorrectness(CXXForRangeStmt *rangeLoop)
{
    TypeUtils::QualTypeClassification classif;
    auto varDecl = rangeLoop->getLoopVariable();
    bool success = TypeUtils::classifyQualType(m_context, varDecl, /*by-ref*/classif, rangeLoop);
    if (!success)
        return;

    if (classif.passNonTriviallyCopyableByConstRef) {
        string msg;
        const string paramStr = clazy::simpleTypeName(varDecl->getType(), lo());
        msg = "Missing reference in range-for with non trivial type (" + paramStr + ')';

        // We ignore classif.passSmallTrivialByValue because it doesn't matter, the compiler is able
        // to optimize it, generating the same assembly, regardless of pass by value.
        emitWarning(varDecl->getLocStart(), msg.c_str());
    }
}
