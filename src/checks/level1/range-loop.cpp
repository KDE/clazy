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
#include "SourceCompatibilityHelpers.h"
#include "FixItUtils.h"
#include "ClazyContext.h"
#include "PreProcessorVisitor.h"

#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/Expr.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/StmtCXX.h>
#include <clang/AST/Type.h>
#include <clang/Basic/LLVM.h>
#include <llvm/Support/Casting.h>

class ClazyContext;

using namespace clang;
using namespace std;

enum Fixit {
    Fixit_AddRef = 1,
    Fixit_AddqAsConst = 2
};

namespace clazy {
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

    if (!valDecl)
        return false;

    const auto context = dyn_cast<FunctionDecl>(valDecl->getDeclContext());
    if (!context)
        return false;

    bodyRange.body = context->getBody();
    if (!bodyRange.body)
        return false;

    if (valDecl->hasInit()) {
        if (auto cleanupExpr = dyn_cast<clang::ExprWithCleanups>(valDecl->getInit())) {
            if (auto ce = dyn_cast<clang::CXXConstructExpr>(cleanupExpr->getSubExpr())) {
                if (!ce->isListInitialization() && !ce->isStdInitListInitialization()) {
                    // When initing via copy or move ctor there's possible detachments.
                    return false;
                }
            }
        }
    }

    // TODO1: Being passed to a function as const should be OK
    if (Utils::isPassedToFunction(bodyRange, valDecl, false))
        return false;

    return true;
}
}


RangeLoop::RangeLoop(const std::string &name, ClazyContext *context)
    : CheckBase(name, context, Option_CanIgnoreIncludes)
{
    context->enablePreprocessorVisitor();
}

void RangeLoop::VisitStmt(clang::Stmt *stmt)
{
    if (auto rangeLoop = dyn_cast<CXXForRangeStmt>(stmt)) {
        processForRangeLoop(rangeLoop);
    }
}

bool RangeLoop::islvalue(Expr *exp, SourceLocation &endLoc)
{
    if (isa<DeclRefExpr>(exp)) {
        endLoc = clazy::locForEndOfToken(&m_astContext, clazy::getLocStart(exp));
        return true;
    }

    if (auto me = dyn_cast<MemberExpr>(exp)) {
        auto decl = me->getMemberDecl();
        if (!decl || isa<FunctionDecl>(decl))
            return false;

        endLoc = clazy::locForEndOfToken(&m_astContext, me->getMemberLoc());
        return true;
    }

    return false;
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
    if (!clazy::unrefQualType(loopVariableType).isConstQualified() && loopVariableType->isReferenceType())
        return;

    CXXRecordDecl *record = t->getAsCXXRecordDecl();
    if (!clazy::isQtCOWIterableClass(Utils::rootBaseClass(record)))
        return;

    StmtBodyRange bodyRange(nullptr, &sm(), clazy::getLocStart(rangeLoop));
    if (clazy::containerNeverDetaches(clazy::containerDeclForLoop(rangeLoop), bodyRange))
        return;

    std::vector<FixItHint> fixits;

    SourceLocation end;
    if (islvalue(containerExpr, /*by-ref*/end)) {
        PreProcessorVisitor *preProcessorVisitor = m_context->preprocessorVisitor;
        if (!preProcessorVisitor || preProcessorVisitor->qtVersion() >= 50700) { // qAsConst() was added to 5.7
            SourceLocation start = clazy::getLocStart(containerExpr);
            fixits.push_back(clazy::createInsertion(start, "qAsConst("));
            //SourceLocation end = getLocEnd(containerExpr);
            fixits.push_back(clazy::createInsertion(end, ")"));
        }
    }

    emitWarning(clazy::getLocStart(rangeLoop), "c++11 range-loop might detach Qt container (" + record->getQualifiedNameAsString() + ')', fixits);
}

void RangeLoop::checkPassByConstRefCorrectness(CXXForRangeStmt *rangeLoop)
{
    clazy::QualTypeClassification classif;
    auto varDecl = rangeLoop->getLoopVariable();
    bool success = varDecl && clazy::classifyQualType(m_context, varDecl->getType(), varDecl, /*by-ref*/ classif, rangeLoop);
    if (!success)
        return;

    if (classif.passNonTriviallyCopyableByConstRef) {
        string msg;
        const string paramStr = clazy::simpleTypeName(varDecl->getType(), lo());
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
