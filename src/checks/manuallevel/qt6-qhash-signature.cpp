/*
    This file is part of the clazy static checker.

    Copyright (C) 2020 The Qt Company Ltd.
    Copyright (C) 2020 Lucie Gerard <lucie.gerard@qt.io>

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

#include "qt6-qhash-signature.h"
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

Qt6QHashSignature::Qt6QHashSignature(const std::string &name, ClazyContext *context)
    : CheckBase(name, context, Option_CanIgnoreIncludes)
{
}

static bool isInterestingFunction(std::string name) {
    if (name == "qHash" || name == "qHashBits" || name == "qHashRange" || name == "qHashRangeCommutative")
        return true;
    return false;
}

static int uintToSizetParam(clang::FunctionDecl *funcDecl)
{
    std::string functionName = funcDecl->getNameAsString();
    // the uint signature is on the second parameter for the qHash function
    // it is on the third paramater for qHashBits, qHashRange and qHashCommutative
   if (functionName == "qHash" && funcDecl->getNumParams() == 2)
        return 1;
    if ((functionName ==  "qHashBits" || functionName == "qHashRange" || functionName == "qHashRangeCommutative")
            && funcDecl->getNumParams() == 3)
        return 2;

    return -1;
}

static clang::ParmVarDecl* getInterestingParam(clang::FunctionDecl *funcDecl)
{
    if (uintToSizetParam(funcDecl)>0)
        return funcDecl->getParamDecl(uintToSizetParam(funcDecl));
    return NULL;
}

static bool isWrongParamType(clang::FunctionDecl *funcDecl)
{
    // Second or third parameter of the qHash functions should be size_t
    auto param = getInterestingParam(funcDecl);
    if (!param)
        return false;
    const string typeStr = param->getType().getAsString();
    if (typeStr != "size_t") {
        return true;
    }
    return false;
}

static bool isWrongReturnType(clang::FunctionDecl *funcDecl)
{
    if (!funcDecl)
        return false;

    //Return type should be size_t
    if (funcDecl->getReturnType().getAsString() != "size_t")
        return true;
    return false;
}

void Qt6QHashSignature::VisitStmt(clang::Stmt *stmt)
{
    DeclRefExpr *declRefExpr = dyn_cast<DeclRefExpr>(stmt);
    if (!declRefExpr)
        return;

    // checking if we're dealing with a qhash function
    std::string name = declRefExpr->getNameInfo().getAsString();
    if (!isInterestingFunction(name))
        return;

    VarDecl *varDecl = m_context->lastDecl ? dyn_cast<VarDecl>(m_context->lastDecl) : nullptr;
    FieldDecl *fieldDecl = m_context->lastDecl ? dyn_cast<FieldDecl>(m_context->lastDecl) : nullptr;
    FunctionDecl *funcDecl = m_context->lastDecl ? dyn_cast<FunctionDecl>(m_context->lastFunctionDecl) : nullptr;

    if (!varDecl && !fieldDecl && !funcDecl)
        return;

    // need to check if this stmt is part of a return stmt, if it is, it's the lastFunctionDecl that is of interest
    // loop over the parent until I find a return statement.
    Stmt *parent = clazy::parent(m_context->parentMap, stmt);
    bool isPartReturnStmt = false;
    if (parent) {
        while (parent) {
            Stmt* ancester = clazy::parent(m_context->parentMap, parent);
            if (!ancester)
                break;
            ReturnStmt *returnStmt = dyn_cast<ReturnStmt>(ancester);
            if (returnStmt) {
                isPartReturnStmt = true;
                break;
            }
            parent = ancester;
        }
    }

    // if the stmt is part of a return statement but there is no last functionDecl, it's a problem...
    if (isPartReturnStmt && !funcDecl)
        return;

    std::string message;
    std::string declType;
    SourceRange fixitRange;
    SourceLocation warningLocation;

    if (funcDecl && isPartReturnStmt) {
        // if the return correspond to a qHash function, we are not interested
        // the qHash function is taken care of during the VisitDecl
        if (isInterestingFunction(funcDecl->getNameAsString()))
            return;
        declType = funcDecl->getReturnType().getAsString();
        fixitRange = funcDecl->getReturnTypeSourceRange();
        warningLocation = clazy::getLocStart(funcDecl);
    } else if (varDecl) {
        declType = varDecl->getType().getAsString();
        fixitRange = varDecl->getTypeSourceInfo()->getTypeLoc().getSourceRange();
        warningLocation = clazy::getLocStart(varDecl);
    } else if (fieldDecl) {
        declType = fieldDecl->getType().getAsString();
        fixitRange = fieldDecl->getTypeSourceInfo()->getTypeLoc().getSourceRange();
        warningLocation = clazy::getLocStart(fieldDecl);
    }

    std::string qhashReturnType = declRefExpr->getDecl()->getAsFunction()->getReturnType().getAsString();
    if (declType == "size_t" && qhashReturnType == "size_t")
        return;

    vector<FixItHint> fixits;

    // if the type of the variable is correct, but the qHash is not returning the right type
    // just emit warning...
    if (declType == "size_t" && qhashReturnType != "size_t") {
        message = name + " should return size_t";
        emitWarning(clazy::getLocStart(declRefExpr), message, fixits);
        return;
    }

    fixits.push_back(FixItHint::CreateReplacement(fixitRange, "size_t"));

    if (qhashReturnType != "size_t") {
        message = name + " should return size_t";
    } else {
        message = name + " returns size_t";
    }
    emitWarning(warningLocation, message, fixits);

    return;
}

void Qt6QHashSignature::VisitDecl(clang::Decl *decl)
{
    FunctionDecl *funcDecl = dyn_cast<FunctionDecl>(decl);
    if (funcDecl) {
        if (!isInterestingFunction(funcDecl->getNameAsString()))
            return;

        bool wrongReturnType = isWrongReturnType(funcDecl);
        bool wrongParamType = isWrongParamType(funcDecl);
        if (!wrongReturnType && !wrongParamType)
            return;
        vector<FixItHint> fixits;
        string message;
        message = funcDecl->getNameAsString() + " with uint signature";
        fixits = fixitReplace(funcDecl, wrongReturnType, wrongParamType);
        emitWarning(clazy::getLocStart(funcDecl), message, fixits);
    }

    return;
}

std::vector<FixItHint> Qt6QHashSignature::fixitReplace(clang::FunctionDecl *funcDecl, bool changeReturnType, bool changeParamType)
{   
    std::string replacement = "";
    vector<FixItHint> fixits;
    if (!funcDecl)
        return fixits;

    if (changeReturnType) {
        replacement = "size_t";
        fixits.push_back(FixItHint::CreateReplacement(funcDecl->getReturnTypeSourceRange(), replacement));
    }
    if (changeParamType) {
        clang::SourceRange range = getInterestingParam(funcDecl)->getTypeSourceInfo()->getTypeLoc().getSourceRange();
        replacement = "size_t";
        fixits.push_back(FixItHint::CreateReplacement(range, replacement));
    }
    return fixits;
}
