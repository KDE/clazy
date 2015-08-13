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

#include "qstringuneededheapallocations.h"
#include "Utils.h"

#include <clang/AST/DeclCXX.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/Expr.h>

using namespace clang;
using namespace std;

QStringUneededHeapAllocations::QStringUneededHeapAllocations(clang::CompilerInstance &ci)
    : CheckBase(ci)
{
}

void QStringUneededHeapAllocations::VisitStmt(clang::Stmt *stm)
{
    VisitCtor(stm);
    VisitOperatorCall(stm);
    VisitFromLatin1OrUtf8(stm);
}

std::string QStringUneededHeapAllocations::name() const
{
    return "qstring-uneeded-heap-allocations";
}

// Returns if the method has only one argument and it's char*
static bool method_has_ctor_with_char_pointer_arg(CXXMethodDecl *methodDecl, string &paramType)
{
    if (methodDecl->param_size() != 1)
        return false;

    ParmVarDecl *firstParm = *methodDecl->param_begin();
    QualType qt = firstParm->getType();

    if (qt.getAsString() == "class QLatin1String") {
        paramType = "QLatin1String";
        return true;
    }

    const Type *t = qt.getTypePtrOrNull();
    if (t == nullptr)
        return false;

    if (t->getPointeeType().getTypePtrOrNull() == nullptr)
        return false;

    if (!t->getPointeeType().getTypePtrOrNull()->isCharType())
        return false;

    paramType = "const char*";
    return true;
}


void QStringUneededHeapAllocations::VisitCtor(Stmt *stm)
{
    CXXConstructExpr *ctorExpr = dyn_cast<CXXConstructExpr>(stm);
    if (ctorExpr == nullptr)
        return;

    std::vector<StringLiteral*> stringLiterals;
    Utils::getChilds2<StringLiteral>(ctorExpr, stringLiterals);

    //  We're only after string literals, str.contains(some_method_returning_const_char_is_fine())
    if (stringLiterals.empty())
        return;

    CXXConstructorDecl *ctorDecl = ctorExpr->getConstructor();
    CXXRecordDecl *recordDecl = ctorDecl->getParent();
    if (recordDecl->getNameAsString() != "QString")
        return;

    string paramType;
    if (!method_has_ctor_with_char_pointer_arg(ctorDecl, paramType))
        return;

    string msg = string("QString(") + paramType + string(") being called [-Wmore-warnings-qstring-uneeded-heap-allocations]");
    emitWarning(stm->getLocStart(), msg.c_str());
}

void QStringUneededHeapAllocations::VisitOperatorCall(Stmt *stm)
{
    CXXOperatorCallExpr *operatorCall = dyn_cast<CXXOperatorCallExpr>(stm);
    if (operatorCall == nullptr)
        return;

    std::vector<StringLiteral*> stringLiterals;
    Utils::getChilds2<StringLiteral>(operatorCall, stringLiterals);

    //  We're only after string literals, str.contains(some_method_returning_const_char_is_fine())
    if (stringLiterals.empty())
        return;

    FunctionDecl *funcDecl = operatorCall->getDirectCallee();
    if (funcDecl == nullptr)
        return;

    CXXMethodDecl *methodDecl = dyn_cast<CXXMethodDecl>(funcDecl);
    if (methodDecl == nullptr || methodDecl->getParent()->getNameAsString() != "QString")
        return;

    string paramType;
    if (!method_has_ctor_with_char_pointer_arg(methodDecl, paramType) || paramType == "QLatin1String")
        return;

    string msg = string("QString(") + paramType + string(") being called [-Wmore-warnings-qstring-uneeded-heap-allocations]");
    emitWarning(stm->getLocStart(), msg.c_str());
}

void QStringUneededHeapAllocations::VisitFromLatin1OrUtf8(Stmt *stmt)
{
    CallExpr *callExpr = dyn_cast<CallExpr>(stmt);
    if (callExpr == nullptr)
        return;

    FunctionDecl *functionDecl = callExpr->getDirectCallee();
    if (functionDecl == nullptr)
        return;

    CXXMethodDecl *methodDecl = dyn_cast<CXXMethodDecl>(functionDecl);
    if (methodDecl == nullptr)
        return;

    std::string functionName = functionDecl->getNameAsString();
    if (functionName != "fromLatin1" && functionName != "fromUtf8")
        return;

    if (methodDecl->getParent()->getNameAsString() != "QString")
        return;

    std::vector<StringLiteral*> stringLiterals;
    Utils::getChilds2<StringLiteral>(callExpr, stringLiterals, /*depth=*/ 2);
    if (stringLiterals.empty())
        return;

    if (functionName == "fromLatin1") {
        emitWarning(stmt->getLocStart(), "QString::fromLatin1() being passed a literal [-Wmore-warnings-qstring-uneeded-heap-allocations]");
    } else {
        emitWarning(stmt->getLocStart(), "QString::fromUtf8() being passed a literal [-Wmore-warnings-qstring-uneeded-heap-allocations]");
    }
}
