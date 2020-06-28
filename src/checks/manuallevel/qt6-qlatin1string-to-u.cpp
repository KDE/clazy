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

#include "qt6-qlatin1string-to-u.h"
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

Qt6QLatin1StringToU::Qt6QLatin1StringToU(const std::string &name, ClazyContext *context)
    : CheckBase(name, context, Option_CanIgnoreIncludes)
{
    enablePreProcessorCallbacks();
}

static bool isInterestingParam(ParmVarDecl *param)
{
    const string typeStr = param->getType().getAsString();
    if (typeStr == "const char *") {// We only want const char*
        return true;
    }

    return false;
}

static bool isInterestingCtorCall(CXXConstructorDecl *ctor)
{
    if (!ctor || !clazy::isOfClass(ctor, "QLatin1String"))
        return false;

    for (auto param : Utils::functionParameters(ctor)) {
        if (isInterestingParam(param))
            return true;
        return false;
    }
    return false;
}

void Qt6QLatin1StringToU::VisitStmt(clang::Stmt *stmt)
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
        message = "QLatin1String(const char *) ctor being called";
        for (auto macro_pos : m_listingMacroExpand) {
            if (m_sm.isPointWithin(macro_pos, clazy::getLocStart(stmt), clazy::getLocEnd(stmt))) {
               emitWarning(clazy::getLocStart(stmt), message, fixits);
               return;
            }
        }
        fixits = fixitReplace(stmt);
    } else {
        return;
    }

    emitWarning(clazy::getLocStart(stmt), message, fixits);
}

std::vector<FixItHint> Qt6QLatin1StringToU::fixitReplace(clang::Stmt *stmt)
{   
    string replacement = "";

    // Iterating over the stmt's children to build the replacement
    int i = 0;
    Stmt *current_stm = stmt;
    for (auto it = current_stm->child_begin() ; it !=current_stm->child_end() ; it++) {
        Stmt *child = clazy::childAt(current_stm, i);
        if (!child)
            break;
        ConditionalOperator *parent_condOp = dyn_cast<ConditionalOperator>(current_stm);

        ImplicitCastExpr *child_dynCastExp = dyn_cast<ImplicitCastExpr>(child);
        StringLiteral *child_stringliteral = dyn_cast<StringLiteral>(child);
        ConditionalOperator *child_condOp = dyn_cast<ConditionalOperator>(child);
        CXXBoolLiteralExpr *child_boolLitExp = dyn_cast<CXXBoolLiteralExpr>(child);
        DeclRefExpr *child_declRefExp = dyn_cast<DeclRefExpr>(child);

         if (child_dynCastExp || child_condOp) {// skipping those.
            current_stm = child;
            i = 0;
        } else if (child_stringliteral) {
            replacement += "u\"";
            replacement += child_stringliteral->getString();
            replacement += "\"";
            if (parent_condOp && i == 1) // the first string is the second child of the ConditionalOperator.
                replacement += " : ";
            i++;
        } else if (child_boolLitExp) {
            replacement = child_boolLitExp->getValue() ? "true" : "false";
            if (parent_condOp)
                    replacement += " ? ";
            i++;
        } else if (child_declRefExp) { // not replacing those cases.
             // would have to check that the ancestor is a QString, and that the type is something accepted by
             // the QString constructor.
             // The NameInfo would then be the replacement.
            return {};
        } else {
            if (it == current_stm->child_end()) {
                current_stm = child;
                i = 0;
            } else {
                i++;
            }
        }
    }

    if (replacement.size() < 3)
        return {};

    vector<FixItHint> fixits;
    fixits.push_back(FixItHint::CreateReplacement(stmt->getSourceRange(), replacement));
    return fixits;
}

void Qt6QLatin1StringToU::VisitMacroExpands(const clang::Token &MacroNameTok, const clang::SourceRange &range, const MacroInfo *)
{
    m_listingMacroExpand.push_back(range.getBegin());
    return;
}
