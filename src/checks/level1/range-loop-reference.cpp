/*
    SPDX-FileCopyrightText: 2015 Klarälvdalens Datakonsult AB a KDAB Group company info@kdab.com
    SPDX-FileContributor: Sérgio Martins <sergio.martins@kdab.com>

    SPDX-FileCopyrightText: 2015 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "range-loop-reference.h"
#include "ClazyContext.h"
#include "FixItUtils.h"
#include "PreProcessorVisitor.h"
#include "QtUtils.h"
#include "StringUtils.h"
#include "TypeUtils.h"

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
    if (auto *rangeLoop = dyn_cast<CXXForRangeStmt>(stmt)) {
        processForRangeLoop(rangeLoop);
    }
}

void RangeLoopReference::processForRangeLoop(CXXForRangeStmt *rangeLoop)
{
    Expr *containerExpr = rangeLoop->getRangeInit();
    if (!containerExpr) {
        return;
    }

    QualType qt = containerExpr->getType();
    const Type *t = qt.getTypePtrOrNull();
    if (!t || !t->isRecordType()) {
        return;
    }

    clazy::QualTypeClassification classif;
    auto *varDecl = rangeLoop->getLoopVariable();
    bool success = varDecl && clazy::classifyQualType(m_context, varDecl->getType(), varDecl, /*by-ref*/ classif, rangeLoop);
    if (!success) {
        return;
    }

    if (classif.passNonTriviallyCopyableByConstRef) {
        std::string msg;
        const std::string paramStr = clazy::simpleTypeName(varDecl->getType(), lo());
        msg = "Missing reference in range-for with non trivial type (" + paramStr + ')';

        std::vector<FixItHint> fixits;
        const bool isConst = varDecl->getType().isConstQualified();

        if (!isConst) {
            SourceLocation start = varDecl->getBeginLoc();
            fixits.push_back(clazy::createInsertion(start, "const "));
        }

        SourceLocation end = varDecl->getLocation();
        fixits.push_back(clazy::createInsertion(end, "&"));

        // We ignore classif.passSmallTrivialByValue because it doesn't matter, the compiler is able
        // to optimize it, generating the same assembly, regardless of pass by value.
        emitWarning(varDecl->getBeginLoc(), msg, fixits);
    }
}
