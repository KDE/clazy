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

#include "range-loop-reference.h"
#include "QtUtils.h"
#include "TypeUtils.h"
#include "StringUtils.h"
#include "SourceCompatibilityHelpers.h"
#include "FixItUtils.h"
#include "ClazyContext.h"
#include "PreProcessorVisitor.h"

#include <clang/AST/Expr.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/StmtCXX.h>
#include <clang/AST/Type.h>
#include <llvm/Support/Casting.h>

class ClazyContext;

using namespace clang;

RangeLoopReference::RangeLoopReference(const std::string &name, ClazyContext *context)
    : CheckBase(name, context, Option_CanIgnoreIncludes)
{
    context->enablePreprocessorVisitor();
}

void RangeLoopReference::VisitStmt(clang::Stmt *stmt)
{
    if (auto rangeLoop = dyn_cast<CXXForRangeStmt>(stmt)) {
        processForRangeLoop(rangeLoop);
    }
}

void RangeLoopReference::processForRangeLoop(CXXForRangeStmt *rangeLoop)
{
    Expr *containerExpr = rangeLoop->getRangeInit();
    if (!containerExpr)
        return;

    QualType qt = containerExpr->getType();
    const Type *t = qt.getTypePtrOrNull();
    if (!t || !t->isRecordType())
        return;

    clazy::QualTypeClassification classif;
    auto varDecl = rangeLoop->getLoopVariable();
    bool success = varDecl && clazy::classifyQualType(m_context, varDecl->getType(), varDecl, /*by-ref*/ classif, rangeLoop);
    if (!success)
        return;

    if (classif.passNonTriviallyCopyableByConstRef) {
        std::string msg;
        const std::string paramStr = clazy::simpleTypeName(varDecl->getType(), lo());
        msg = "Missing reference in range-for with non trivial type (" + paramStr + ')';

        std::vector<FixItHint> fixits;
        const bool isConst = varDecl->getType().isConstQualified();

        if (!isConst) {
            SourceLocation start = clazy::getLocStart(varDecl);
            fixits.push_back(clazy::createInsertion(start, "const "));
        }

        SourceLocation end = varDecl->getLocation();
        fixits.push_back(clazy::createInsertion(end, "&"));


        // We ignore classif.passSmallTrivialByValue because it doesn't matter, the compiler is able
        // to optimize it, generating the same assembly, regardless of pass by value.
        emitWarning(clazy::getLocStart(varDecl), msg.c_str(), fixits);
    }
}
