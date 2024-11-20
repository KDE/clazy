/*
    SPDX-FileCopyrightText: 2015 Klarälvdalens Datakonsult AB a KDAB Group company info@kdab.com
    SPDX-FileContributor: Sérgio Martins <sergio.martins@kdab.com>
    SPDX-FileCopyrightText: 2015-2016 Sergio Martins <smartins@kde.org>
    SPDX-FileCopyrightText: 2024 Alexander Lohnau <alexander.lohnau@gmx.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "reserve-candidates.h"
#include "ClazyContext.h"
#include "ContextUtils.h"
#include "HierarchyUtils.h"
#include "LoopUtils.h"
#include "MacroUtils.h"
#include "QtUtils.h"
#include "StringUtils.h"
#include "Utils.h"
#include "clazy_stl.h"

#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/StmtCXX.h>
#include <clang/AST/Type.h>
#include <clang/Basic/LLVM.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/SourceManager.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Casting.h>

#include <vector>

using namespace clang;

ReserveCandidates::ReserveCandidates(const std::string &name, ClazyContext *context)
    : CheckBase(name, context, Option_CanIgnoreIncludes)
{
}

static bool paramIsSameTypeAs(const Type *paramType, CXXRecordDecl *classDecl)
{
    if (!paramType || !classDecl) {
        return false;
    }

    if (paramType->getAsCXXRecordDecl() == classDecl) {
        return true;
    }

    const CXXRecordDecl *paramClassDecl = paramType->getPointeeCXXRecordDecl();
    return paramClassDecl && paramClassDecl == classDecl;
}

static bool isCandidateMethod(CXXMethodDecl *methodDecl)
{
    if (!methodDecl) {
        return false;
    }

    CXXRecordDecl *classDecl = methodDecl->getParent();
    if (!classDecl) {
        return false;
    }

    if (!clazy::equalsAny(static_cast<std::string>(clazy::name(methodDecl)), {"append", "push_back", "push", "operator<<", "operator+="})) {
        return false;
    }

    if (!clazy::isAReserveClass(classDecl)) {
        return false;
    }

    // Catch cases like: QList<T>::append(const QList<T> &), which don't make sense to reserve.
    // In this case, the parameter has the same type of the class
    ParmVarDecl *parm = methodDecl->getParamDecl(0);
    return !paramIsSameTypeAs(parm->getType().getTypePtrOrNull(), classDecl);
}

static bool isCandidate(CallExpr *oper)
{
    if (!oper) {
        return false;
    }

    return isCandidateMethod(dyn_cast_or_null<CXXMethodDecl>(oper->getDirectCallee()));
}

bool ReserveCandidates::containerWasReserved(clang::ValueDecl *valueDecl) const
{
    return valueDecl && clazy::contains(m_foundReserves, valueDecl);
}

bool ReserveCandidates::acceptsValueDecl(ValueDecl *valueDecl) const
{
    // Rules:
    // 1. The container variable must have been defined inside a function. Too many false positives otherwise.
    //      free to comment that out and go through the results, maybe you'll find something.

    // 2. If we found at least one reserve call, lets not warn about it.

    if (!valueDecl || isa<ParmVarDecl>(valueDecl) || containerWasReserved(valueDecl)) {
        return false;
    }

    if (clazy::isValueDeclInFunctionContext(valueDecl)) {
        return true;
    }

    // Actually, lets allow for some member variables containers if they are being used inside CTORs or DTORs
    // Those functions are only called once, so it's OK. For other member functions it's dangerous and needs
    // human inspection, if such member function would be called in a loop we would be constantly calling reserve
    // and in that case the built-in exponential growth is better.

    if (!m_context->lastMethodDecl || !(isa<CXXConstructorDecl>(m_context->lastMethodDecl) || isa<CXXDestructorDecl>(m_context->lastMethodDecl))) {
        return false;
    }

    const CXXRecordDecl *record = Utils::isMemberVariable(valueDecl);
    return record && m_context->lastMethodDecl->getParent() == record;
}

bool ReserveCandidates::isReserveCandidate(ValueDecl *valueDecl, Stmt *loopBody, CallExpr *callExpr) const
{
    if (!acceptsValueDecl(valueDecl)) {
        return false;
    }

    const bool isMemberVariable = Utils::isMemberVariable(valueDecl);
    // We only want containers defined outside of the loop we're examining
    if (!isMemberVariable && sm().isBeforeInSLocAddrSpace(loopBody->getBeginLoc(), valueDecl->getBeginLoc())) {
        return false;
    }

    if (isInComplexLoop(callExpr, valueDecl->getBeginLoc(), isMemberVariable)) {
        return false;
    }

    if (clazy::loopCanBeInterrupted(loopBody, m_context->sm, callExpr->getBeginLoc())) {
        return false;
    }

    return true;
}

void ReserveCandidates::VisitStmt(clang::Stmt *stm)
{
    if (registerReserveStatement(stm)) {
        return;
    }

    auto *body = clazy::bodyFromLoop(stm);
    if (!body) {
        return;
    }

    const bool isForeach = clazy::isInMacro(&m_astContext, stm->getBeginLoc(), "Q_FOREACH");

    // If the body is another loop, we have nesting, ignore it now since the inner loops will be visited soon.
    if (isa<DoStmt>(body) || isa<WhileStmt>(body) || (!isForeach && isa<ForStmt>(body))) {
        return;
    }

    // TODO: Search in both branches of the if statement
    if (isa<IfStmt>(body)) {
        return;
    }

    // Get the list of member calls and operator<< that are direct childs of the loop statements
    // If it's inside an if statement we don't care.
    auto callExprs = clazy::getStatements<CallExpr>(body,
                                                    nullptr,
                                                    {},
                                                    /*depth=*/1,
                                                    /*includeParent=*/true,
                                                    clazy::IgnoreExprWithCleanups);

    for (CallExpr *callExpr : callExprs) {
        if (!isCandidate(callExpr)) {
            continue;
        }

        ValueDecl *valueDecl = Utils::valueDeclForCallExpr(callExpr);
        if (isReserveCandidate(valueDecl, body, callExpr)) {
            emitWarning(callExpr->getBeginLoc(), "Reserve candidate");
        }
    }
}

// Catch existing reserves
bool ReserveCandidates::registerReserveStatement(Stmt *stm)
{
    auto *memberCall = dyn_cast<CXXMemberCallExpr>(stm);
    if (!memberCall) {
        return false;
    }

    CXXMethodDecl *methodDecl = memberCall->getMethodDecl();
    if (!methodDecl || clazy::name(methodDecl) != "reserve") {
        return false;
    }

    CXXRecordDecl *decl = methodDecl->getParent();
    if (!clazy::isAReserveClass(decl)) {
        return false;
    }

    ValueDecl *valueDecl = Utils::valueDeclForMemberCall(memberCall);
    if (!valueDecl) {
        return false;
    }

    if (!clazy::contains(m_foundReserves, valueDecl)) {
        m_foundReserves.push_back(valueDecl);
    }

    return true;
}

bool ReserveCandidates::expressionIsComplex(clang::Expr *expr) const
{
    if (!expr) {
        return false;
    }

    std::vector<CallExpr *> callExprs;
    clazy::getChilds<CallExpr>(expr, callExprs);

    for (CallExpr *callExpr : callExprs) {
        // In Qt5, this would have been a BinaryOperator. Ignore iterator unequality checks here
        if (auto operatorCall = dyn_cast<CXXOperatorCallExpr>(callExpr)) {
            std::string name = operatorCall->getDirectCallee()->getAsFunction()->getQualifiedNameAsString();
            if (clazy::contains(name, "iterator::operator")) {
                continue;
            }
        }

        if (clazy::isJavaIterator(dyn_cast<CXXMemberCallExpr>(callExpr))) {
            continue;
        }

        QualType qt = callExpr->getType();
        const Type *t = qt.getTypePtrOrNull();
        if (t && (!t->isIntegerType() || t->isBooleanType())) {
            return true;
        }
    }

    std::vector<ArraySubscriptExpr *> subscriptExprs;
    clazy::getChilds<ArraySubscriptExpr>(expr, subscriptExprs);
    if (!subscriptExprs.empty()) {
        return true;
    }

    auto *binary = dyn_cast<BinaryOperator>(expr);
    if (binary && binary->isAssignmentOp()) { // Filter things like for ( ...; ...; next = node->next)

        Expr *rhs = binary->getRHS();
        if (isa<MemberExpr>(rhs) || (isa<ImplicitCastExpr>(rhs) && dyn_cast_or_null<MemberExpr>(clazy::getFirstChildAtDepth(rhs, 1)))) {
            return true;
        }
    }

    // llvm::errs() << expr->getStmtClassName() << "\n";
    return false;
}

bool ReserveCandidates::loopIsComplex(clang::Stmt *stm, bool &isLoop) const
{
    isLoop = false;

    if (auto *forstm = dyn_cast<ForStmt>(stm)) {
        isLoop = true;
        return !forstm->getCond() || !forstm->getInc() || expressionIsComplex(forstm->getCond()) || expressionIsComplex(forstm->getInc());
    }

    if (isa<CXXForRangeStmt>(stm)) {
        isLoop = true;
        return false;
    }

    if (dyn_cast<DoStmt>(stm) || dyn_cast<WhileStmt>(stm)) {
        // Too many false-positives with while statements. Ignore it.
        isLoop = true;
        return true;
    }

    return false;
}

bool ReserveCandidates::isInComplexLoop(clang::Stmt *s, SourceLocation declLocation, bool isMemberVariable) const
{
    if (!s || declLocation.isInvalid()) {
        return false;
    }

    int forCount = 0;
    int foreachCount = 0;

    static std::vector<unsigned int> nonComplexOnesCache;
    static std::vector<unsigned int> complexOnesCache;
    auto rawLoc = s->getBeginLoc().getRawEncoding();

    // For some reason we generate two warnings on some foreaches, so cache the ones we processed
    // and return true so we don't trigger a warning
    if (clazy::contains(nonComplexOnesCache, rawLoc) || clazy::contains(complexOnesCache, rawLoc)) {
        return true;
    }

    Stmt *parent = s;
    PresumedLoc lastForeachForStm;
    while ((parent = clazy::parent(m_context->parentMap, parent))) {
        const SourceLocation parentStart = parent->getBeginLoc();
        if (!isMemberVariable && sm().isBeforeInSLocAddrSpace(parentStart, declLocation)) {
            nonComplexOnesCache.push_back(rawLoc);
            return false;
        }

        bool isLoop = false;
        if (loopIsComplex(parent, isLoop)) {
            complexOnesCache.push_back(rawLoc);
            return true;
        }

        if (clazy::isInForeach(&m_astContext, parentStart)) {
            auto ploc = sm().getPresumedLoc(parentStart);
            if (Utils::presumedLocationsEqual(ploc, lastForeachForStm)) {
                // Q_FOREACH comes in pairs, because each has two for statements inside, so ignore one when counting
            } else {
                foreachCount++;
                lastForeachForStm = ploc;
            }
        } else {
            if (isLoop) {
                forCount++;
            }
        }

        if (foreachCount > 1 || forCount > 1) { // two foreaches are almost always a false-positve
            complexOnesCache.push_back(rawLoc);
            return true;
        }
    }

    nonComplexOnesCache.push_back(rawLoc);
    return false;
}
