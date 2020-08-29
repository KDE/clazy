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

#include "qt6-qlatin1char-to-u.h"
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

Qt6QLatin1CharToU::Qt6QLatin1CharToU(const std::string &name, ClazyContext *context)
    : CheckBase(name, context, Option_CanIgnoreIncludes)
{
    enablePreProcessorCallbacks();
}

static bool isInterestingCtorCall(CXXConstructorDecl *ctor)
{
    if (!ctor || !clazy::isOfClass(ctor, "QLatin1Char"))
        return false;

    for (auto param : Utils::functionParameters(ctor)) {
        return param->getType().getTypePtr()->isCharType();
    }
    return false;
}

void Qt6QLatin1CharToU::VisitStmt(clang::Stmt *stmt)
{
    auto ctorExpr = dyn_cast<CXXConstructExpr>(stmt);
    if (!ctorExpr)
        return;

    vector<FixItHint> fixits;
    string message;

    if (ctorExpr) {
        CXXConstructorDecl *ctorDecl = ctorExpr->getConstructor();
        if (!isInterestingCtorCall(ctorDecl))
            return;
        message = "QLatin1Char(char) ctor being called";
        for (auto macro_pos : m_listingMacroExpand) {
            if (m_sm.isPointWithin(macro_pos, clazy::getLocStart(stmt), clazy::getLocEnd(stmt))) {
               emitWarning(clazy::getLocStart(stmt), message, fixits);
               return;
            }
        }
        bool noFix = false;
        std::string replacement  = buildReplacement(stmt, noFix);
        if (!noFix) {
            fixits.push_back(FixItHint::CreateReplacement(stmt->getSourceRange(), replacement));
        }
    } else {
        return;
    }

    emitWarning(clazy::getLocStart(stmt), message, fixits);
}

std::string Qt6QLatin1CharToU::buildReplacement(clang::Stmt *stmt, bool &noFix, bool ancestorIsCondition,
                                                int ancestorConditionChildNumber)
{
    std::string replacement;
    Stmt *current_stmt = stmt;

    int i = 0;

    for (auto it = current_stmt->child_begin() ; it !=current_stmt->child_end() ; it++) {

        Stmt *child = *it;
        ConditionalOperator *parent_condOp = dyn_cast<ConditionalOperator>(current_stmt);
        ConditionalOperator *child_condOp = dyn_cast<ConditionalOperator>(child);

        if (parent_condOp) {
            ancestorIsCondition = true;
            ancestorConditionChildNumber = i;
            if (ancestorConditionChildNumber == 2)
                replacement += " : ";
        }

        // to handle nested condition
        if (child_condOp && ancestorIsCondition) {
            replacement += "(";
        }

        replacement += buildReplacement(child, noFix, ancestorIsCondition, ancestorConditionChildNumber);

        DeclRefExpr *child_declRefExp = dyn_cast<DeclRefExpr>(child);
        CXXBoolLiteralExpr *child_boolLitExp = dyn_cast<CXXBoolLiteralExpr>(child);
        CharacterLiteral *child_charliteral = dyn_cast<CharacterLiteral>(child);

        if (child_charliteral) {
            replacement += "u\'";
            if (child_charliteral->getValue() == 92 || child_charliteral->getValue() == 39)
                replacement += "\\";
            replacement += child_charliteral->getValue();
            replacement += "\'";
        } else if (child_boolLitExp) {
            replacement = child_boolLitExp->getValue() ? "true" : "false";
                    replacement += " ? ";
        } else if (child_declRefExp) {
            if (ancestorIsCondition && ancestorConditionChildNumber == 0
                    && child_declRefExp->getType().getAsString() == "_Bool") {
                replacement += child_declRefExp->getNameInfo().getAsString();
                replacement += " ? ";
            } else {
                // not supporting those cases
                noFix = true;
                return {};
            }
       } else if (child_condOp && ancestorIsCondition) {
            replacement += ")";
       }

        i++;
    }
    return replacement;
}

void Qt6QLatin1CharToU::VisitMacroExpands(const clang::Token &MacroNameTok, const clang::SourceRange &range, const MacroInfo *)
{
    m_listingMacroExpand.push_back(range.getBegin());
    return;
}
