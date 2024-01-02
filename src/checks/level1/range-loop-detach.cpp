/*
    This file is part of the clazy static checker.

    SPDX-FileCopyrightText: 2015 Klarälvdalens Datakonsult AB a KDAB Group company info@kdab.com
    Author: Sérgio Martins <sergio.martins@kdab.com>

    SPDX-FileCopyrightText: 2015 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "range-loop-detach.h"
#include "ClazyContext.h"
#include "FixItUtils.h"
#include "LoopUtils.h"
#include "PreProcessorVisitor.h"
#include "QtUtils.h"
#include "SourceCompatibilityHelpers.h"
#include "StmtBodyRange.h"
#include "StringUtils.h"
#include "TypeUtils.h"
#include "Utils.h"

#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/Expr.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/StmtCXX.h>
#include <clang/AST/Type.h>
#include <llvm/Support/Casting.h>

class ClazyContext;

using namespace clang;

namespace clazy
{
/**
 * Returns true if we can prove the container doesn't detach.
 * Returns false otherwise, meaning that you can't conclude anything if false is returned.
 *
 * For true to be returned, all these conditions must verify:
 * - Container is a local variable
 * - It's not passed to any function
 * - It's not assigned to another variable
 */
bool containerNeverDetaches(const clang::VarDecl *valDecl, StmtBodyRange bodyRange) // clazy:exclude=function-args-by-value
{
    // This helps for bug 367485

    if (!valDecl) {
        return false;
    }

    const auto *const context = dyn_cast<FunctionDecl>(valDecl->getDeclContext());
    if (!context) {
        return false;
    }

    bodyRange.body = context->getBody();
    if (!bodyRange.body) {
        return false;
    }

    if (valDecl->hasInit()) {
        if (const auto *cleanupExpr = dyn_cast<clang::ExprWithCleanups>(valDecl->getInit())) {
            if (const auto *ce = dyn_cast<clang::CXXConstructExpr>(cleanupExpr->getSubExpr())) {
                if (!ce->isListInitialization() && !ce->isStdInitListInitialization()) {
                    // When initing via copy or move ctor there's possible detachments.
                    return false;
                }
            } else if (auto *bte = dyn_cast<clang::CXXBindTemporaryExpr>(cleanupExpr->getSubExpr())) {
                return false;
            }
        }
    }

    // TODO1: Being passed to a function as const should be OK
    if (Utils::isPassedToFunction(bodyRange, valDecl, false)) {
        return false;
    }

    return true;
}
}

RangeLoopDetach::RangeLoopDetach(const std::string &name, ClazyContext *context)
    : CheckBase(name, context, Option_CanIgnoreIncludes)
{
    context->enablePreprocessorVisitor();
}

void RangeLoopDetach::VisitStmt(clang::Stmt *stmt)
{
    if (auto *rangeLoop = dyn_cast<CXXForRangeStmt>(stmt)) {
        processForRangeLoop(rangeLoop);
    }
}

bool RangeLoopDetach::islvalue(Expr *exp, SourceLocation &endLoc)
{
    if (isa<DeclRefExpr>(exp)) {
        endLoc = clazy::locForEndOfToken(&m_astContext, clazy::getLocStart(exp));
        return true;
    }

    if (auto *me = dyn_cast<MemberExpr>(exp)) {
        auto *decl = me->getMemberDecl();
        if (!decl || isa<FunctionDecl>(decl)) {
            return false;
        }

        endLoc = clazy::locForEndOfToken(&m_astContext, me->getMemberLoc());
        return true;
    }

    return false;
}

void RangeLoopDetach::processForRangeLoop(CXXForRangeStmt *rangeLoop)
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

    if (qt.isConstQualified()) { // const won't detach
        return;
    }

    auto loopVariableType = rangeLoop->getLoopVariable()->getType();
    if (!clazy::unrefQualType(loopVariableType).isConstQualified() && loopVariableType->isReferenceType()) {
        return;
    }

    CXXRecordDecl *record = t->getAsCXXRecordDecl();
    if (!clazy::isQtCOWIterableClass(Utils::rootBaseClass(record))) {
        return;
    }

    StmtBodyRange bodyRange(nullptr, &sm(), clazy::getLocStart(rangeLoop));
    if (clazy::containerNeverDetaches(clazy::containerDeclForLoop(rangeLoop), bodyRange)) {
        return;
    }

    std::vector<FixItHint> fixits;

    SourceLocation end;
    if (islvalue(containerExpr, /*by-ref*/ end)) {
        PreProcessorVisitor *preProcessorVisitor = m_context->preprocessorVisitor;
        if (!preProcessorVisitor || preProcessorVisitor->qtVersion() >= 50700) { // qAsConst() was added to 5.7
            SourceLocation start = clazy::getLocStart(containerExpr);
            fixits.push_back(clazy::createInsertion(start, "qAsConst("));
            // SourceLocation end = getLocEnd(containerExpr);
            fixits.push_back(clazy::createInsertion(end, ")"));
        }
    }

    emitWarning(clazy::getLocStart(rangeLoop), "c++11 range-loop might detach Qt container (" + record->getQualifiedNameAsString() + ')', fixits);
}
