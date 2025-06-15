/*
    SPDX-FileCopyrightText: 2020 The Qt Company Ltd.
    SPDX-FileCopyrightText: 2020 Lucie Gerard <lucie.gerard@qt.io>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "qt6-qhash-signature.h"
#include "ClazyContext.h"
#include "FixItUtils.h"
#include "HierarchyUtils.h"
#include "Utils.h"

#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/Type.h>
#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/LLVM.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Lex/Lexer.h>
#include <llvm/ADT/ArrayRef.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Casting.h>

using namespace clang;

Qt6QHashSignature::Qt6QHashSignature(const std::string &name, ClazyContext *context)
    : CheckBase(name, context, Option_CanIgnoreIncludes)
{
}

static bool isInterestingFunction(const std::string &name)
{
    return name == "qHash" || name == "qHashBits" || name == "qHashRange" || name == "qHashRangeCommutative";
}

static int uintToSizetParam(clang::FunctionDecl *funcDecl)
{
    std::string functionName = funcDecl->getNameAsString();
    // the uint signature is on the second parameter for the qHash function
    // it is on the third parameter for qHashBits, qHashRange and qHashCommutative
    if (functionName == "qHash" && funcDecl->getNumParams() == 2) {
        return 1;
    }
    if ((functionName == "qHashBits" || functionName == "qHashRange" || functionName == "qHashRangeCommutative") && funcDecl->getNumParams() == 3) {
        return 2;
    }

    return -1;
}

static clang::ParmVarDecl *getInterestingParam(clang::FunctionDecl *funcDecl)
{
    if (uintToSizetParam(funcDecl) > 0) {
        return funcDecl->getParamDecl(uintToSizetParam(funcDecl));
    }
    return nullptr;
}

static bool isWrongParamType(clang::FunctionDecl *funcDecl)
{
    // Second or third parameter of the qHash functions should be size_t
    auto *param = getInterestingParam(funcDecl);
    if (!param) {
        return false;
    }
    const std::string typeStr = param->getType().getAsString();
    return typeStr != "size_t";
}

static bool isWrongReturnType(clang::FunctionDecl *funcDecl)
{
    if (!funcDecl) {
        return false;
    }

    // Return type should be size_t
    if (funcDecl->getReturnType().getAsString() != "size_t") {
        return true;
    }
    return false;
}

void Qt6QHashSignature::VisitStmt(clang::Stmt *stmt)
{
    auto *declRefExpr = dyn_cast<DeclRefExpr>(stmt);
    if (!declRefExpr) {
        return;
    }

    // checking if we're dealing with a qhash function
    std::string name = declRefExpr->getNameInfo().getAsString();
    if (!isInterestingFunction(name)) {
        return;
    }
    if (!m_context->lastDecl) {
        return;
    }

    VarDecl *varDecl = dyn_cast<VarDecl>(m_context->lastDecl);
    FieldDecl *fieldDecl = dyn_cast<FieldDecl>(m_context->lastDecl);
    FunctionDecl *funcDecl = m_context->lastFunctionDecl;

    if (!varDecl && !fieldDecl && !funcDecl) {
        return;
    }

    // need to check if this stmt is part of a return stmt, if it is, it's the lastFunctionDecl that is of interest
    // loop over the parent until I find a return statement.
    Stmt *parent = clazy::parent(m_context->parentMap, stmt);
    bool isPartReturnStmt = false;
    if (parent) {
        while (parent) {
            Stmt *ancestor = clazy::parent(m_context->parentMap, parent);
            if (!ancestor) {
                break;
            }
            auto *returnStmt = dyn_cast<ReturnStmt>(ancestor);
            if (returnStmt) {
                isPartReturnStmt = true;
                break;
            }
            parent = ancestor;
        }
    }

    // if the stmt is part of a return statement but there is no last functionDecl, it's a problem...
    if (isPartReturnStmt && !funcDecl) {
        return;
    }

    std::string message;
    std::string declType;
    SourceRange fixitRange;
    SourceLocation warningLocation;

    if (funcDecl && isPartReturnStmt) {
        // if the return correspond to a qHash function, we are not interested
        // the qHash function is taken care of during the VisitDecl
        if (isInterestingFunction(funcDecl->getNameAsString())) {
            return;
        }
        declType = funcDecl->getReturnType().getAsString();
        fixitRange = funcDecl->getReturnTypeSourceRange();
        warningLocation = funcDecl->getBeginLoc();
    } else if (varDecl) {
        declType = varDecl->getType().getAsString();
        fixitRange = varDecl->getTypeSourceInfo()->getTypeLoc().getSourceRange();
        warningLocation = varDecl->getBeginLoc();
    } else if (fieldDecl) {
        declType = fieldDecl->getType().getAsString();
        fixitRange = fieldDecl->getTypeSourceInfo()->getTypeLoc().getSourceRange();
        warningLocation = fieldDecl->getBeginLoc();
    }

    std::string qhashReturnType = declRefExpr->getDecl()->getAsFunction()->getReturnType().getAsString();
    if (declType == "size_t" && qhashReturnType == "size_t") {
        return;
    }

    std::vector<FixItHint> fixits;

    // if the type of the variable is correct, but the qHash is not returning the right type
    // just emit warning...
    if (declType == "size_t" && qhashReturnType != "size_t") {
        message = name + " should return size_t";
        emitWarning(declRefExpr->getBeginLoc(), message, fixits);
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
    auto *funcDecl = dyn_cast<FunctionDecl>(decl);
    if (funcDecl) {
        if (!isInterestingFunction(funcDecl->getNameAsString())) {
            return;
        }

        bool wrongReturnType = isWrongReturnType(funcDecl);
        bool wrongParamType = isWrongParamType(funcDecl);
        if (!wrongReturnType && !wrongParamType) {
            return;
        }
        std::vector<FixItHint> fixits;
        std::string message;
        message = funcDecl->getNameAsString() + " with uint signature";
        fixits = fixitReplace(funcDecl, wrongReturnType, wrongParamType);
        emitWarning(funcDecl->getBeginLoc(), message, fixits);
    }

    return;
}

std::vector<FixItHint> Qt6QHashSignature::fixitReplace(clang::FunctionDecl *funcDecl, bool changeReturnType, bool changeParamType)
{
    std::string replacement;
    std::vector<FixItHint> fixits;
    if (!funcDecl) {
        return fixits;
    }

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
