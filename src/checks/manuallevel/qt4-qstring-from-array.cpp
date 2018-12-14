/*
    This file is part of the clazy static checker.

    Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
    Author: Sérgio Martins <sergio.martins@kdab.com>

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

#include "qt4-qstring-from-array.h"
#include "ClazyContext.h"
#include "Utils.h"
#include "StringUtils.h"
#include "FixItUtils.h"
#include "HierarchyUtils.h"
#include "SourceCompatibilityHelpers.h"
#include "clazy_stl.h"

#include <clang/Lex/Lexer.h>
#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/Type.h>
#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/LLVM.h>
#include <clang/Basic/SourceLocation.h>
#include <llvm/ADT/ArrayRef.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Casting.h>

using namespace clang;
using namespace std;

Qt4QStringFromArray::Qt4QStringFromArray(const std::string &name, ClazyContext *context)
    : CheckBase(name, context, Option_CanIgnoreIncludes)
{
}

static bool isInterestingParam(ParmVarDecl *param, bool &is_char_array, bool &is_byte_array)
{
    is_char_array = false;
    is_byte_array = false;
    const string typeStr = param->getType().getAsString();
    if (typeStr == "const class QByteArray &") {
        is_byte_array = true;
    } else if (typeStr == "const char *") {// We only want bytearray and const char*
        is_char_array = true;
    }

    return is_char_array || is_byte_array;
}

static bool isInterestingCtorCall(CXXConstructorDecl *ctor, bool &is_char_array, bool &is_byte_array)
{
    is_char_array = false;
    is_byte_array = false;
    if (!ctor || !clazy::isOfClass(ctor, "QString"))
        return false;

    for (auto param : Utils::functionParameters(ctor)) {
        if (isInterestingParam(param, is_char_array, is_byte_array))
            break;

        return false;
    }

    return is_char_array || is_byte_array;
}

static bool isInterestingMethod(const string &methodName)
{
    static const vector<string> methods = { "append", "prepend", "operator=", "operator==", "operator!=", "operator<", "operator<=", "operator>", "operator>=", "operator+=" };
    return clazy::contains(methods, methodName);
}

static bool isInterestingMethodCall(CXXMethodDecl *method, string &methodName, bool &is_char_array, bool &is_byte_array)
{
    is_char_array = false;
    is_byte_array = false;
    if (!method)
        return false;

    if (clazy::name(method->getParent()) != "QString" || method->getNumParams() != 1)
        return false;

    methodName = method->getNameAsString();
    if (!isInterestingMethod(methodName))
        return false;

    if (!isInterestingParam(method->getParamDecl(0), is_char_array, is_byte_array))
        return false;

    return true;
}

static bool isInterestingOperatorCall(CXXOperatorCallExpr *op, string &operatorName, bool &is_char_array, bool &is_byte_array)
{
    is_char_array = false;
    is_byte_array = false;
    FunctionDecl *func = op->getDirectCallee();
    if (!func)
        return false;

    return isInterestingMethodCall(dyn_cast<CXXMethodDecl>(func), operatorName, is_char_array, is_byte_array);
}

void Qt4QStringFromArray::VisitStmt(clang::Stmt *stm)
{
    CXXConstructExpr *ctorExpr = dyn_cast<CXXConstructExpr>(stm);
    CXXOperatorCallExpr *operatorCall = dyn_cast<CXXOperatorCallExpr>(stm);
    CXXMemberCallExpr *memberCall = dyn_cast<CXXMemberCallExpr>(stm);
    if (!ctorExpr && !operatorCall && !memberCall)
        return;

    vector<FixItHint> fixits;
    bool is_char_array = false;
    bool is_byte_array = false;
    string methodName;
    string message;

    if (ctorExpr) {
        CXXConstructorDecl *ctorDecl = ctorExpr->getConstructor();

        if (!isInterestingCtorCall(ctorDecl, is_char_array, is_byte_array))
            return;

        fixits = fixCtorCall(ctorExpr);
        if (is_char_array) {
            message = "QString(const char *) ctor being called";
        } else {
            message = "QString(QByteArray) ctor being called";
        }
    } else if (operatorCall) {
        if (!isInterestingOperatorCall(operatorCall, /*by-ref*/ methodName, is_char_array, is_byte_array))
            return;

        fixits = fixOperatorCall(operatorCall);
    } else if (memberCall) {
        if (!isInterestingMethodCall(memberCall->getMethodDecl(), /*by-ref*/ methodName, is_char_array, is_byte_array))
            return;

        fixits = fixMethodCallCall(memberCall);
    } else {
        return;
    }

    if (operatorCall || memberCall) {
        if (is_char_array) {
            message = "QString::" + methodName + "(const char *) being called";
        } else {
            message = "QString::" + methodName + "(QByteArray) being called";
        }
    }

    emitWarning(clazy::getLocStart(stm), message, fixits);
}

std::vector<FixItHint> Qt4QStringFromArray::fixCtorCall(CXXConstructExpr *ctorExpr)
{
    Stmt *parent = clazy::parent(m_context->parentMap, ctorExpr); // CXXBindTemporaryExpr
    Stmt *grandParent = clazy::parent(m_context->parentMap, parent); //CXXFunctionalCastExpr

    if (parent && grandParent && isa<CXXBindTemporaryExpr>(parent) && isa<CXXFunctionalCastExpr>(grandParent)) {
        return fixitReplaceWithFromLatin1(ctorExpr);
    } else {
        return fixitInsertFromLatin1(ctorExpr);
    }
}

std::vector<FixItHint> Qt4QStringFromArray::fixOperatorCall(CXXOperatorCallExpr *op)
{
    vector<FixItHint> fixits;
    if (op->getNumArgs() == 2) {
        Expr *e = op->getArg(1);
        SourceLocation start = clazy::getLocStart(e);
        SourceLocation end = Lexer::getLocForEndOfToken(clazy::biggestSourceLocationInStmt(sm(), e), 0, sm(), lo());

        SourceRange range = { start, end };
        if (range.isInvalid()) {
            emitWarning(clazy::getLocStart(op), "internal error");
            return {};
        }

        clazy::insertParentMethodCall("QString::fromLatin1", {start, end}, /*by-ref*/ fixits);

    } else {
        emitWarning(clazy::getLocStart(op), "internal error");
    }


    return fixits;
}

std::vector<FixItHint> Qt4QStringFromArray::fixMethodCallCall(clang::CXXMemberCallExpr *memberExpr)
{
    vector<FixItHint> fixits;

    if (memberExpr->getNumArgs() == 1) {
        Expr *e = *(memberExpr->arg_begin());
        SourceLocation start = clazy::getLocStart(e);
        SourceLocation end = Lexer::getLocForEndOfToken(clazy::biggestSourceLocationInStmt(sm(), e), 0, sm(), lo());

        SourceRange range = { start, end };
        if (range.isInvalid()) {
            emitWarning(clazy::getLocStart(memberExpr), "internal error");
            return {};
        }

        clazy::insertParentMethodCall("QString::fromLatin1", {start, end}, /*by-ref*/ fixits);
    } else {
        emitWarning(clazy::getLocStart(memberExpr), "internal error");
    }


    return fixits;
}

std::vector<FixItHint> Qt4QStringFromArray::fixitReplaceWithFromLatin1(CXXConstructExpr *ctorExpr)
{
    const string replacement = "QString::fromLatin1";
    const string replacee = "QString";
    vector<FixItHint> fixits;

    SourceLocation rangeStart = clazy::getLocStart(ctorExpr);
    SourceLocation rangeEnd = Lexer::getLocForEndOfToken(rangeStart, -1, sm(), lo());

    if (rangeEnd.isInvalid()) {
        // Fallback. Have seen a case in the wild where the above would fail, it's very rare
        rangeEnd = rangeStart.getLocWithOffset(replacee.size() - 2);
        if (rangeEnd.isInvalid()) {
            clazy::printLocation(sm(), rangeStart);
            clazy::printLocation(sm(), rangeEnd);
            clazy::printLocation(sm(), Lexer::getLocForEndOfToken(rangeStart, 0, sm(), lo()));
            queueManualFixitWarning(clazy::getLocStart(ctorExpr));
            return {};
        }
    }

    fixits.push_back(FixItHint::CreateReplacement(SourceRange(rangeStart, rangeEnd), replacement));
    return fixits;
}

std::vector<FixItHint> Qt4QStringFromArray::fixitInsertFromLatin1(CXXConstructExpr *ctorExpr)
{
    vector<FixItHint> fixits;
    SourceRange range;

    Expr *arg = *(ctorExpr->arg_begin());
    range.setBegin(clazy::getLocStart(arg));
    range.setEnd(Lexer::getLocForEndOfToken(clazy::biggestSourceLocationInStmt(sm(), ctorExpr), 0, sm(), lo()));
    if (range.isInvalid()) {
        emitWarning(clazy::getLocStart(ctorExpr), "Internal error");
        return {};
    }

    clazy::insertParentMethodCall("QString::fromLatin1", range, fixits);

    return fixits;
}
