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

#include "reservecandidates.h"
#include "Utils.h"
#include "clazy_stl.h"
#include "MacroUtils.h"
#include "checkmanager.h"
#include "StringUtils.h"
#include "HierarchyUtils.h"

#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/DeclTemplate.h>

#include <vector>

using namespace clang;
using namespace std;

ReserveCandidates::ReserveCandidates(const std::string &name, const clang::CompilerInstance &ci)
    : CheckBase(name, ci)
{
}

static bool isAReserveClass(CXXRecordDecl *recordDecl)
{
    if (!recordDecl)
        return false;

    static const std::vector<std::string> classes = {"QVector", "vector", "QList", "QSet", "QVarLengthArray"};

    for (auto it = classes.cbegin(), end = classes.cend(); it != end; ++it) {
        if (Utils::derivesFrom(recordDecl, *it))
            return true;
    }

    return false;
}

static bool paramIsSameTypeAs(const Type *paramType, CXXRecordDecl *classDecl)
{
    if (!paramType || !classDecl)
        return false;

    if (paramType->getAsCXXRecordDecl() == classDecl)
        return true;

    const CXXRecordDecl *paramClassDecl = paramType->getPointeeCXXRecordDecl();
    return paramClassDecl && paramClassDecl == classDecl;
}

static bool isCandidateMethod(CXXMethodDecl *methodDecl)
{
    if (!methodDecl)
        return false;

    CXXRecordDecl *classDecl = methodDecl->getParent();
    if (!classDecl)
        return false;

    auto methodName = methodDecl->getNameAsString();
    if (methodName != "append" && methodName != "push_back" && methodName != "push" /*&& methodName != "insert"*/)
        return false;

    if (!isAReserveClass(classDecl))
        return false;

    // Catch cases like: QList<T>::append(const QList<T> &), which don't make sense to reserve.
    // In this case, the parameter has the same type of the class
    ParmVarDecl *parm = methodDecl->getParamDecl(0);
    if (paramIsSameTypeAs(parm->getType().getTypePtrOrNull(), classDecl))
        return false;

    return true;
}

static bool isCandidateOperator(CXXOperatorCallExpr *oper)
{
    if (!oper)
        return false;

    auto calleeDecl = dyn_cast_or_null<CXXMethodDecl>(oper->getDirectCallee());
    if (!calleeDecl)
        return false;

    const std::string operatorName = calleeDecl->getNameAsString();
    if (operatorName != "operator<<" && operatorName != "operator+=")
        return false;

    CXXRecordDecl *recordDecl = calleeDecl->getParent();
    if (!isAReserveClass(recordDecl))
        return false;

    // Catch cases like: QList<T>::append(const QList<T> &), which don't make sense to reserve.
    // In this case, the parameter has the same type of the class
    ParmVarDecl *parm = calleeDecl->getParamDecl(0);
    if (paramIsSameTypeAs(parm->getType().getTypePtrOrNull(), recordDecl))
        return false;

    return true;
}

bool ReserveCandidates::containerWasReserved(clang::ValueDecl *valueDecl) const
{
    return valueDecl && clazy_std::contains(m_foundReserves, valueDecl);
}

bool ReserveCandidates::acceptsValueDecl(ValueDecl *valueDecl) const
{
    // Rules:
    // 1. The container variable must have been defined inside a function. Too many false positives otherwise.
    //      free to comment that out and go through the results, maybe you'll find something.

    // 2. If we found at least one reserve call, lets not warn about it.

    if (!valueDecl || isa<ParmVarDecl>(valueDecl) || containerWasReserved(valueDecl))
        return false;

    if (Utils::isValueDeclInFunctionContext(valueDecl))
        return true;

    // Actually, lets allow for some member variables containers if they are being used inside CTORs or DTORs
    // Those functions are only called once, so it's OK. For other member functions it's dangerous and needs
    // human inspection, if such member function would be called in a loop we would be constantly calling reserve
    // and in that case the built-in exponential growth is better.

    if (!m_lastMethodDecl || !(isa<CXXConstructorDecl>(m_lastMethodDecl) || isa<CXXDestructorDecl>(m_lastMethodDecl)))
        return false;

    CXXRecordDecl *record = Utils::isMemberVariable(valueDecl);
    if (record && m_lastMethodDecl->getParent() == record)
        return true;

    return false;
}

bool ReserveCandidates::isReserveCandidate(ValueDecl *valueDecl, Stmt *loopBody, CallExpr *callExpr) const
{
    if (!acceptsValueDecl(valueDecl))
        return false;

    const bool isMemberVariable = Utils::isMemberVariable(valueDecl);
    // We only want containers defined outside of the loop we're examining
    if (!isMemberVariable && m_ci.getSourceManager().isBeforeInSLocAddrSpace(loopBody->getLocStart(), valueDecl->getLocStart()))
        return false;

    if (isInComplexLoop(callExpr, valueDecl->getLocStart(), isMemberVariable))
        return false;

    if (Utils::loopCanBeInterrupted(loopBody, m_ci, callExpr->getLocStart()))
        return false;

    return true;
}

void ReserveCandidates::VisitStmt(clang::Stmt *stm)
{
    if (registerReserveStatement(stm))
        return;

    auto body = Utils::bodyFromLoop(stm);
    if (!body)
        return;

    const bool isForeach = MacroUtils::isInMacro(m_ci, stm->getLocStart(), "Q_FOREACH");

    // If the body is another loop, we have nesting, ignore it now since the inner loops will be visited soon.
    if (isa<DoStmt>(body) || isa<WhileStmt>(body) || (!isForeach && isa<ForStmt>(body)))
        return;

    // TODO: Search in both branches of the if statement
    if (isa<IfStmt>(body))
        return;

    vector<CXXMemberCallExpr*> callExprs;
    vector<CXXOperatorCallExpr*> operatorCalls;

    // Get the list of member calls and operator<< that are direct childs of the loop statements
    // If it's inside an if statement we don't care.
    HierarchyUtils::getChildsHACK<CXXMemberCallExpr>(body, callExprs);
    HierarchyUtils::getChildsHACK<CXXOperatorCallExpr>(body, operatorCalls); // For operator<<

    for (CXXMemberCallExpr *callExpr : callExprs) {
        if (!isCandidateMethod(callExpr->getMethodDecl()))
            continue;

        ValueDecl *valueDecl = Utils::valueDeclForMemberCall(callExpr);
        if (isReserveCandidate(valueDecl, body, callExpr))
            emitWarning(callExpr->getLocStart(), "Reserve candidate");
    }

    for (CXXOperatorCallExpr *callExpr : operatorCalls) {
        if (!isCandidateOperator(callExpr))
            continue;

        ValueDecl *valueDecl = Utils::valueDeclForOperatorCall(callExpr);
        if (isReserveCandidate(valueDecl, body, callExpr))
            emitWarning(callExpr->getLocStart(), "Reserve candidate");
    }
}

// Catch existing reserves
bool ReserveCandidates::registerReserveStatement(Stmt *stm)
{
    auto memberCall = dyn_cast<CXXMemberCallExpr>(stm);
    if (!memberCall)
        return false;

    CXXMethodDecl *methodDecl = memberCall->getMethodDecl();
    if (!methodDecl || methodDecl->getNameAsString() != "reserve")
        return false;

    CXXRecordDecl *decl = methodDecl->getParent();
    if (!isAReserveClass(decl))
        return false;

    ValueDecl *valueDecl = Utils::valueDeclForMemberCall(memberCall);
    if (!valueDecl)
        return false;

    if (!clazy_std::contains(m_foundReserves, valueDecl)) {
        m_foundReserves.push_back(valueDecl);
    }

    return true;
}

static bool isJavaIterator(CXXMemberCallExpr *call)
{
    if (!call)
        return false;

    static const vector<string> names = {"QHashIterator", "QMapIterator", "QSetIterator", "QListIterator",
                                         "QVectorIterator", "QLinkedListIterator", "QStringListIterator"};
    CXXRecordDecl *record = call->getRecordDecl();
    string name = record == nullptr ? "" : record->getNameAsString();
    return clazy_std::contains(names, name);
}

bool ReserveCandidates::expressionIsTooComplex(clang::Expr *expr) const
{
    if (!expr)
        return false;

    vector<CallExpr*> callExprs;
    HierarchyUtils::getChilds2<CallExpr>(expr, callExprs);

    for (CallExpr *callExpr : callExprs) {
        if (isJavaIterator(dyn_cast<CXXMemberCallExpr>(callExpr)))
            continue;

        QualType qt = callExpr->getType();
        const Type *t = qt.getTypePtrOrNull();
        if (t && (!t->isIntegerType() || t->isBooleanType()))
            return true;
    }

    vector<ArraySubscriptExpr*> subscriptExprs;
    HierarchyUtils::getChilds2<ArraySubscriptExpr>(expr, subscriptExprs);
    if (!subscriptExprs.empty())
        return true;

    BinaryOperator* binary = dyn_cast<BinaryOperator>(expr);
    if (binary && binary->isAssignmentOp()) { // Filter things like for ( ...; ...; next = node->next)

        Expr *rhs = binary->getRHS();
        if (isa<MemberExpr>(rhs) || (isa<ImplicitCastExpr>(rhs) && dyn_cast_or_null<MemberExpr>(HierarchyUtils::getFirstChildAtDepth(rhs, 1))))
            return true;
    }

    // llvm::errs() << expr->getStmtClassName() << "\n";
    return false;
}

bool ReserveCandidates::loopIsTooComplex(clang::Stmt *stm, bool &isLoop) const
{
    isLoop = false;
    auto forstm = dyn_cast<ForStmt>(stm);
    if (forstm) {
        isLoop = true;
        return forstm->getCond() == nullptr || forstm->getInc() == nullptr || expressionIsTooComplex(forstm->getCond()) || expressionIsTooComplex(forstm->getInc());
    }

    auto whilestm = dyn_cast<WhileStmt>(stm);
    if (whilestm) {
        isLoop = true;
        return true; // Too many false-positives with while statement. Ignore it.
        //return expressionIsTooComplex(whilestm->getCond());
    }

    auto dostm = dyn_cast<DoStmt>(stm);
    if (dostm) {
        isLoop = true;
        return true; // Too many false-positives with do-while statement. Ignore it.
        //return expressionIsTooComplex(dostm->getCond());
    }

    return false;
}

bool ReserveCandidates::isInComplexLoop(clang::Stmt *s, SourceLocation declLocation, bool isMemberVariable) const
{
    if (!s || declLocation.isInvalid())
        return false;

    int loopCount = 0;
    int foreachCount = 0;

    static vector<unsigned int> nonComplexOnesCache;
    static vector<unsigned int> complexOnesCache;
    auto rawLoc = s->getLocStart().getRawEncoding();


    // For some reason we generate two warnings on some foreaches, so cache the ones we processed
    // and return true so we don't trigger a warning
    if (clazy_std::contains(nonComplexOnesCache, rawLoc))
        return true;

    if (clazy_std::contains(complexOnesCache, rawLoc))
        return true;

    Stmt *it = s;
    PresumedLoc lastForeachForStm;
    while (Stmt *parent = HierarchyUtils::parent(m_parentMap, it)) {
        const SourceLocation parentStart = parent->getLocStart();
        if (!isMemberVariable && m_ci.getSourceManager().isBeforeInSLocAddrSpace(parentStart, declLocation)) {
            nonComplexOnesCache.push_back(rawLoc);
            return false;
        }

        bool isLoop = false;
        if (loopIsTooComplex(parent, isLoop)) {
            complexOnesCache.push_back(rawLoc);
            return true;
        }

        if (isLoop)
            loopCount++;

        if (MacroUtils::isInMacro(m_ci, parentStart, "Q_FOREACH")) {
            auto ploc = m_ci.getSourceManager().getPresumedLoc(parentStart);
            if (Utils::presumedLocationsEqual(ploc, lastForeachForStm)) {
                // Q_FOREACH comes in pairs, because each has two for statements inside, so ignore one when counting
            } else {
                foreachCount++;
                lastForeachForStm = ploc;
            }
        }

        if (foreachCount > 1) { // two foreaches are almost always a false-positve
            complexOnesCache.push_back(rawLoc);
            return true;
        }

        if (loopCount >= 4) {
            complexOnesCache.push_back(rawLoc);
            return true;
        }

        it = parent;
    }

    nonComplexOnesCache.push_back(rawLoc);
    return false;
}

REGISTER_CHECK_WITH_FLAGS("reserve-candidates", ReserveCandidates, CheckLevel2)
