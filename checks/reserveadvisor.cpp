/**********************************************************************
**  Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
**  Author: Sérgio Martins <sergio.martins@kdab.com>
**
** This file may be distributed and/or modified under the terms of the
** GNU Lesser General Public License version 2.1 and version 3 as published by the
** Free Software Foundation and appearing in the file LICENSE.LGPL.txt included.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**********************************************************************/

#include "reserveadvisor.h"
#include "Utils.h"

#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/DeclTemplate.h>

#include <vector>

using namespace clang;
using namespace std;

ReserveAdvisor::ReserveAdvisor(clang::CompilerInstance &ci)
    : CheckBase(ci)
{
}

static bool isAReserveClass(CXXRecordDecl *recordDecl)
{
    if (recordDecl == nullptr)
        return false;

    static const std::vector<std::string> classes = {"QVector", "vector", "QList", "QSet", "QVarLengthArray"};

    auto it = classes.cbegin();
    auto end = classes.cend();
    for (; it != end; ++it) {
        if (Utils::descendsFrom(recordDecl, *it))
            return true;
    }

    return false;
}

static bool paramIsSameTypeAs(const Type *paramType, CXXRecordDecl *classDecl)
{
    if (paramType == nullptr)
        return false;

    if (paramType->getAsCXXRecordDecl() && paramType->getAsCXXRecordDecl() == classDecl)
        return true;

    const CXXRecordDecl *paramClassDecl = paramType->getPointeeCXXRecordDecl();
    return paramClassDecl && paramClassDecl == classDecl;
}

static bool isCandidateMethod(CXXMethodDecl *methodDecl)
{
    if (methodDecl == nullptr)
        return false;

    CXXRecordDecl *classDecl = methodDecl->getParent();
    if (classDecl == nullptr)
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
    if (oper == nullptr)
        return false;

    if (oper->getDirectCallee() == nullptr)
        return false;

    std::string operatorName = "";

    auto calleeDecl = dyn_cast<CXXMethodDecl>(oper->getDirectCallee());
    if (calleeDecl == nullptr)
        return false;

    operatorName = calleeDecl->getNameAsString();
    if (operatorName != "operator<<" && operatorName != "operator+=")
        return false;

    CXXRecordDecl *recordDecl = calleeDecl->getParent();
    if (recordDecl == nullptr)
        return false;

    if (!isAReserveClass(recordDecl))
        return false;

    // Catch cases like: QList<T>::append(const QList<T> &), which don't make sense to reserve.
    // In this case, the parameter has the same type of the class
    ParmVarDecl *parm = calleeDecl->getParamDecl(0);
    if (paramIsSameTypeAs(parm->getType().getTypePtrOrNull(), recordDecl))
        return false;

    return true;
}

bool ReserveAdvisor::containerWasReserved(clang::ValueDecl *valueDecl) const
{
    if (valueDecl == nullptr)
        return false;

    return std::find(m_foundReserves.cbegin(), m_foundReserves.cend(), valueDecl) != m_foundReserves.cend();
}

bool ReserveAdvisor::acceptsValueDecl(ValueDecl *valueDecl) const
{
    // Rules:
    // 1. The container variable must have been defined inside a function. Too many false positives otherwise.
    //      free to comment that out and go through the results, maybe you'll find something.

    // 2. If we found at least one reserve call, lets not warn about it.

    return valueDecl && dyn_cast<ParmVarDecl>(valueDecl) == nullptr && Utils::isValueDeclInFunctionContext(valueDecl) && !containerWasReserved(valueDecl);
}

void ReserveAdvisor::printWarning(const SourceLocation &loc)
{
    emitWarning(loc, "Reserve candidate [-Wmore-warnings-reserve-candidate]");
}

void ReserveAdvisor::VisitStmt(clang::Stmt *stm)
{
    checkIfReserveStatement(stm);

    auto forstm = dyn_cast<ForStmt>(stm);
    auto whilestm = dyn_cast<WhileStmt>(stm);
    auto dostm = dyn_cast<DoStmt>(stm);

    if (!forstm && !whilestm && !dostm)
        return;

    auto body = forstm ? forstm->getBody()
                       : whilestm ? whilestm->getBody()
                                  : dostm->getBody();

    if (body == nullptr || isa<IfStmt>(body) || isa<DoStmt>(body) || isa<WhileStmt>(body))
        return;

    vector<CXXMemberCallExpr*> callExprs;
    vector<CXXOperatorCallExpr*> operatorCalls;

    // Get the list of member calls and operator<< that are direct childs of the loop statements
    // If it's inside an if statement we don't care.
    Utils::getChilds<CXXMemberCallExpr>(body, callExprs);
    Utils::getChilds<CXXOperatorCallExpr>(body, operatorCalls); // For operator<<

    for (CXXMemberCallExpr *callExpr : callExprs) {
        if (!isCandidateMethod(callExpr->getMethodDecl()))
            continue;

        ValueDecl *valueDecl = Utils::valueDeclForMemberCall(callExpr);
        if (!acceptsValueDecl(valueDecl))
            continue;

        if (Utils::loopCanBeInterrupted(body, m_ci, callExpr->getLocStart()))
            continue;

        printWarning(callExpr->getLocStart());
    }

    for (CXXOperatorCallExpr *callExpr : operatorCalls) {
        if (!isCandidateOperator(callExpr))
            continue;

        ValueDecl *valueDecl = Utils::valueDeclForOperatorCall(callExpr);
        if (!acceptsValueDecl(valueDecl))
            continue;

        if (Utils::loopCanBeInterrupted(body, m_ci, callExpr->getLocStart()))
            continue;

        printWarning(callExpr->getLocStart());
    }
}

// Catch existing reserves
void ReserveAdvisor::checkIfReserveStatement(Stmt *stm)
{
    auto memberCall = dyn_cast<CXXMemberCallExpr>(stm);
    if (memberCall == nullptr)
        return;

    CXXMethodDecl *methodDecl = memberCall->getMethodDecl();
    if (methodDecl == nullptr || methodDecl->getNameAsString() != "reserve")
        return;

    CXXRecordDecl *decl = methodDecl->getParent();
    if (decl == nullptr || !isAReserveClass(decl))
        return;

    ValueDecl *valueDecl = Utils::valueDeclForMemberCall(memberCall);
    if (valueDecl == nullptr)
        return;

    if (std::find(m_foundReserves.cbegin(), m_foundReserves.cend(), valueDecl) == m_foundReserves.cend()) {
        m_foundReserves.push_back(valueDecl);
    }
}

std::string ReserveAdvisor::name() const
{
    return "reserve-candidate";
}
