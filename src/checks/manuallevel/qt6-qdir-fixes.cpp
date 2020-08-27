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

#include "qt6-qdir-fixes.h"
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

Qt6QDirFixes::Qt6QDirFixes(const std::string &name, ClazyContext *context)
    : CheckBase(name, context, Option_CanIgnoreIncludes)
{
    enablePreProcessorCallbacks();
}

std::string Qt6QDirFixes::findPathArgument(clang::Stmt *stmt, bool ancestorIsCondition, int ancestorConditionChildNumber)
{
    std::string replacement;
    Stmt *current_stmt = stmt;

    int i = 0;

    for (auto it = current_stmt->child_begin() ; it !=current_stmt->child_end() ; it++) {

        Stmt *child = *it;
        ConditionalOperator *parent_condOp = dyn_cast<ConditionalOperator>(current_stmt);
        ConditionalOperator *child_condOp = dyn_cast<ConditionalOperator>(child);

        /*
        For cases like: dir = cond ? "path1" : "path2";
        simplified AST after the operand= look like this:
        CXXBindTemporaryExpr
            |   `-CXXConstructExpr
            |     `-ConditionalOperator.......................... The ancestor that is a conditional operator
            |       |-ImplicitCastExpr 'bool'........................ ancestorConditionChildNumber == 0
            |       | `-DeclRefExpr  'cond' 'bool'
            |       |-ImplicitCastExpr 'const char *'................ ancestorConditionChildNumber == 1
            |       | `-StringLiteral 'const char []' "path1"    => Need to know it has ConditionalOperator has ancestor
            |       |                                            => Need to know it comes from the second children of this ancestor
            |       |                                            => to put the ':' between the two StringLiteral
            |       `-ImplicitCastExpr 'const char *'................ ancestorConditionChildNumber == 2
            |         `-StringLiteral 'const char []' "path2"

        */
        if (parent_condOp) {
            ancestorIsCondition = true;
            ancestorConditionChildNumber = i;
            if (ancestorConditionChildNumber == 2)
                replacement += ":";
        }

        // to handle nested condition
        if (child_condOp && ancestorIsCondition) {
            replacement += "(";
        }

        replacement += findPathArgument(child, ancestorIsCondition, ancestorConditionChildNumber);

        DeclRefExpr *child_declRefExp = dyn_cast<DeclRefExpr>(child);
        CXXBoolLiteralExpr *child_boolLitExp = dyn_cast<CXXBoolLiteralExpr>(child);
        StringLiteral *child_stringliteral = dyn_cast<StringLiteral>(child);

        if (child_stringliteral) {
            replacement += "\"";
            replacement += child_stringliteral->getString();
            replacement += "\"";
        } else if (child_boolLitExp) {
            replacement = child_boolLitExp->getValue() ? "true" : "false";
                    replacement += " ? ";
        } else if (child_declRefExp) {
            if (ancestorIsCondition && ancestorConditionChildNumber == 0
                    && child_declRefExp->getType().getAsString() == "_Bool") {
                replacement += child_declRefExp->getNameInfo().getAsString();
                replacement += " ? ";
            } else {
                //assuming that the variable is compatible with setPath function.
                // if the code was compiling with dir = variable, should be ok to write dir.setPath(variable)
                replacement += child_declRefExp->getNameInfo().getAsString();
            }
       } else if (child_condOp && ancestorIsCondition) {
            replacement += ")";
       }

        i++;
    }
    return replacement;
}

void Qt6QDirFixes::VisitStmt(clang::Stmt *stmt)
{
    CXXOperatorCallExpr *oppCallExpr = dyn_cast<CXXOperatorCallExpr>(stmt);
    DeclRefExpr *declRefExp = dyn_cast<DeclRefExpr>(stmt);

    SourceLocation warningLocation;
    std::string replacement;
    std::string message;
    std::string declType;
    SourceRange fixitRange;

    vector<FixItHint> fixits;

    if (oppCallExpr) {
        // check if the return type is QDir
        //clang::StringRef returnType = oppCallExpr->getCallReturnType(m_astContext).getAsString();
        //if ( !returnType.contains("class QDir"))
        if ( !clazy::isOfClass(oppCallExpr, "QDir") )
            return;

        // only interested in '=' operator
        Stmt *child = clazy::childAt(stmt, 0);
        while (child) {
            DeclRefExpr *decl = dyn_cast<DeclRefExpr>(child);
            if ( !decl ) {
                child = clazy::childAt(child, 0);
                continue;
            }
            if ( decl->getNameInfo().getAsString() == "operator=" ) {
                warningLocation = decl->getLocation();
                break;
            }
            child = clazy::childAt(child, 0);
        }

        // get the name of the QDir variable from child2 value
        child = clazy::childAt(stmt, 1);
        if (!child)
            return;
        DeclRefExpr *declb = dyn_cast<DeclRefExpr>(child);
        if ( !declb )
            return;
        message = " function setPath(\"\") has to be used in Qt6";
        // need to make sure there is no macro
        for (auto macro_pos : m_listingMacroExpand) {
            if (m_sm.isPointWithin(macro_pos, clazy::getLocStart(stmt), clazy::getLocEnd(stmt))) {
               emitWarning(warningLocation, message, fixits);
               return;
            }
        }
        replacement = declb->getNameInfo().getAsString();
        replacement += ".setPath(";
        replacement += findPathArgument(clazy::childAt(stmt, 2));
        replacement += ");";

        fixitRange = stmt->getSourceRange();

    } else if (declRefExp) {
        if (declRefExp->getNameInfo().getAsString() == "addResourceSearchPath") {
            message = " use function QDir::addSearchPath() with prefix instead";
            warningLocation = declRefExp->getBeginLoc();
            emitWarning(warningLocation, message, fixits);
            return;
        } else {
            return;
        }
    }else {
        return;
    }

    fixits.push_back(FixItHint::CreateReplacement(fixitRange, replacement));
    emitWarning(warningLocation, message, fixits);

    return;
}

void Qt6QDirFixes::VisitMacroExpands(const clang::Token &MacroNameTok, const clang::SourceRange &range, const MacroInfo *)
{
    m_listingMacroExpand.push_back(range.getBegin());
    return;
}
