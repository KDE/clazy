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

#include "qt6-qlatin1stringchar-to-u.h"
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

Qt6QLatin1StringCharToU::Qt6QLatin1StringCharToU(const std::string &name, ClazyContext *context)
    : CheckBase(name, context, Option_CanIgnoreIncludes)
{
    enablePreProcessorCallbacks();
}

static bool isQLatin1CharDecl(CXXConstructorDecl *decl)
{
    if (decl && clazy::isOfClass(decl, "QLatin1Char"))
        return true;
    return false;
}

static bool isQLatin1StringDecl(CXXConstructorDecl *decl)
{
    if (decl && clazy::isOfClass(decl, "QLatin1String"))
        return true;
    return false;
}

/*
 * To be interesting, the CXXContructExpr:
 * 1/ must be of class QLatin1String
 * 2/ must have a CXXFunctionalCastExpr with name QLatin1String
 *    (to pick only one of two the CXXContructExpr of class QLatin1String)
 * 3/ must not be nested within an other QLatin1String call (unless looking for left over)
 *    This is done by looking for CXXFunctionalCastExpr with name QLatin1String among parents
 *    QLatin1String call nesting in other QLatin1String call are treating while visiting the outer call.
 */
static bool isInterestingCtorCall(CXXConstructExpr *ctorExpr, const ClazyContext *const context, bool check_parent = true)
{
    CXXConstructorDecl *ctorDecl = ctorExpr->getConstructor();
    if (!isQLatin1CharDecl(ctorDecl) && !isQLatin1StringDecl(ctorDecl))
        return false;

    Stmt *parent_stmt = clazy::parent(context->parentMap, ctorExpr);
    if (!parent_stmt)
        return false;
    bool oneFunctionalCast = false;
    // A given QLatin1Char call will has two ctorExpr passing the isQLatin1CharDecl
    // To avoid creating multiple fixit in case of nested QLatin1String calls
    // it is important to only test the one right after a CXXFunctionalCastExpr with QLatin1String name
    if (isa<CXXFunctionalCastExpr>(parent_stmt)) {
        CXXFunctionalCastExpr* parent = dyn_cast<CXXFunctionalCastExpr>(parent_stmt);
        if (parent->getConversionFunction()->getNameAsString() != "QLatin1Char"
                && parent->getConversionFunction()->getNameAsString() != "QLatin1String") {
            return false;
        } else {
            oneFunctionalCast = true;
        }
    }

    // Not checking the parent when looking for left over QLatin1String call nested in a QLatin1String call not supporting fixit
    if (!check_parent)
        return oneFunctionalCast;

    parent_stmt = context->parentMap->getParent(parent_stmt);
    // If an other CXXFunctionalCastExpr QLatin1String is found among the parents
    // the present QLatin1String call is nested in an other QLatin1String call and should be ignored.
    // The outer call will take care of it.
    while(parent_stmt) {
        if (isa<CXXFunctionalCastExpr>(parent_stmt)) {
            CXXFunctionalCastExpr* parent = dyn_cast<CXXFunctionalCastExpr>(parent_stmt);
            NamedDecl *ndecl = parent->getConversionFunction();
            if (ndecl) {
                if (ndecl->getNameAsString() == "QLatin1Char" || ndecl->getNameAsString() == "QLatin1String") {
                return false;
                }
            }
        }
        parent_stmt = context->parentMap->getParent(parent_stmt);
    }

    return oneFunctionalCast;
}


void Qt6QLatin1StringCharToU::VisitStmt(clang::Stmt *stmt)
{
    auto ctorExpr = dyn_cast<CXXConstructExpr>(stmt);
    if (!ctorExpr)
        return;
    if (!isInterestingCtorCall(ctorExpr, m_context, true))
        return;

    vector<FixItHint> fixits;
    string message;
    for (auto macro_pos : m_listingMacroExpand) {
        if (m_sm.isPointWithin(macro_pos, clazy::getLocStart(stmt), clazy::getLocEnd(stmt))) {
           message = "QLatin1Char or QLatin1String is being called (fix it not supported because of macro)";
           emitWarning(clazy::getLocStart(stmt), message, fixits);
           return;
        }
    }
    checkCTorExpr(stmt, true);
}

bool Qt6QLatin1StringCharToU::checkCTorExpr(clang::Stmt *stmt, bool check_parents)
{
    auto ctorExpr = dyn_cast<CXXConstructExpr>(stmt);
    if (!ctorExpr)
        return false;

    vector<FixItHint> fixits;
    string message;

    // parents are not check when looking inside a QLatin1Char that does not support fixes
    // extra paratheses might be needed for the inner QLatin1Char fix
    bool extra_parentheses = !check_parents;

    bool noFix = false;
    if (ctorExpr) {
        if (!isInterestingCtorCall(ctorExpr, m_context, check_parents))
            return false;
        message = "QLatin1Char or QLatin1String is being called";
        std::string replacement = buildReplacement(stmt, noFix, extra_parentheses);
        if (!noFix) {
            fixits.push_back(FixItHint::CreateReplacement(stmt->getSourceRange(), replacement));
        }

    } else {
        return false;
    }

    emitWarning(clazy::getLocStart(stmt), message, fixits);

    if (noFix) {
        lookForLeftOver(stmt);
    }

    return true;
}

void Qt6QLatin1StringCharToU::lookForLeftOver(clang::Stmt *stmt, bool keep_looking)
{
    Stmt *current_stmt = stmt;
    for (auto it = current_stmt->child_begin() ; it !=current_stmt->child_end() ; it++) {

        Stmt *child = *it;
        // if QLatin1Char is found, stop looking into children of current child
        // the QLatin1Char calls present there, if any, will be caught
        keep_looking = !checkCTorExpr(child, false);
        if (keep_looking)
            lookForLeftOver(child, keep_looking);
    }
}

std::string Qt6QLatin1StringCharToU::buildReplacement(clang::Stmt *stmt, bool &noFix, bool extra, bool ancestorIsCondition,
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

        // to handle catching left over nested QLatin1String call
        if (extra && child_condOp && !ancestorIsCondition) {
            replacement += "(";
        }

        replacement += buildReplacement(child, noFix, extra, ancestorIsCondition, ancestorConditionChildNumber);

        DeclRefExpr *child_declRefExp = dyn_cast<DeclRefExpr>(child);
        CXXBoolLiteralExpr *child_boolLitExp = dyn_cast<CXXBoolLiteralExpr>(child);
        CharacterLiteral *child_charliteral = dyn_cast<CharacterLiteral>(child);
        StringLiteral *child_stringliteral = dyn_cast<StringLiteral>(child);

        if (child_stringliteral) {
            replacement += "u\"";
            replacement += child_stringliteral->getString();
            replacement += "\"";
        } else if (child_charliteral) {
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

        if (extra && child_condOp && !ancestorIsCondition) {
            replacement += ")";
        }

        i++;
    }
    return replacement;
}

void Qt6QLatin1StringCharToU::VisitMacroExpands(const clang::Token &MacroNameTok, const clang::SourceRange &range, const MacroInfo *)
{
    m_listingMacroExpand.push_back(range.getBegin());
    return;
}
