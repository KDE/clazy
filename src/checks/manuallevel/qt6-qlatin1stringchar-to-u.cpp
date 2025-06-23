/*
    SPDX-FileCopyrightText: 2020 The Qt Company Ltd.
    SPDX-FileCopyrightText: 2020 Lucie Gerard <lucie.gerard@qt.io>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "qt6-qlatin1stringchar-to-u.h"
#include "ClazyContext.h"
#include "FixItUtils.h"
#include "HierarchyUtils.h"
#include "StringUtils.h"
#include "Utils.h"
#include "clazy_stl.h"

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

Qt6QLatin1StringCharToU::Qt6QLatin1StringCharToU(const std::string &name)
    : CheckBase(name, Option_CanIgnoreIncludes)
{
}

static bool isQLatin1CharDecl(CXXConstructorDecl *decl)
{
    return decl && clazy::isOfClass(decl, "QLatin1Char");
}

static bool isQLatin1StringDecl(CXXConstructorDecl *decl)
{
    return decl && clazy::isOfClass(decl, "QLatin1String");
}

bool Qt6QLatin1StringCharToU::foundQCharOrQString(Stmt *stmt)
{
    QualType type;
    if (auto *init = dyn_cast<InitListExpr>(stmt)) {
        type = init->getType();
    } else if (auto *opp = dyn_cast<CXXOperatorCallExpr>(stmt)) {
        type = opp->getType();
    } else if (auto *constr = dyn_cast<CXXConstructExpr>(stmt)) {
        type = constr->getType();
    } else if (auto *decl = dyn_cast<DeclRefExpr>(stmt)) {
        type = decl->getType();
    } else if (auto *func = dyn_cast<CXXFunctionalCastExpr>(stmt)) {
        type = func->getType();
    } else if (dyn_cast<CXXMemberCallExpr>(stmt)) {
        Stmt *child = clazy::childAt(stmt, 0);
        while (child) {
            if (foundQCharOrQString(child)) {
                return true;
            }
            child = clazy::childAt(child, 0);
        }
    }

    if (auto *ptr = type.getTypePtrOrNull(); !ptr || (!ptr->isRecordType() && !ptr->isConstantArrayType())) {
        return false;
    }
    std::string typeStr = type.getAsString(lo());
    return typeStr.find("QString") != std::string::npos || typeStr.find("QChar") != std::string::npos;
}

bool Qt6QLatin1StringCharToU::relatedToQStringOrQChar(Stmt *stmt, const ClazyContext *const context)
{
    if (!stmt) {
        return false;
    }

    while (stmt) {
        if (foundQCharOrQString(stmt)) {
            return true;
        }

        stmt = clazy::parent(context->parentMap, stmt);
    }

    return false;
}

/*
 * To be interesting, the CXXContructExpr:
 * 1/ must be of class QLatin1String
 * 2/ must have a CXXFunctionalCastExpr with name QLatin1String
 *    (to pick only one of two the CXXContructExpr of class QLatin1String)
 * 3/ must not be nested within an other QLatin1String call (unless looking for left over)
 *    This is done by looking for CXXFunctionalCastExpr with name QLatin1String among parents
 *    QLatin1String call nesting in other QLatin1String call are treated while visiting the outer call.
 */
bool Qt6QLatin1StringCharToU::isInterestingCtorCall(CXXConstructExpr *ctorExpr, const ClazyContext *const context, bool check_parent)
{
    CXXConstructorDecl *ctorDecl = ctorExpr->getConstructor();
    if (!isQLatin1CharDecl(ctorDecl) && !isQLatin1StringDecl(ctorDecl)) {
        return false;
    }

    Stmt *parent_stmt = clazy::parent(context->parentMap, ctorExpr);
    if (!parent_stmt) {
        return false;
    }
    bool oneFunctionalCast = false;
    // A given QLatin1Char/String call will have two ctorExpr passing the isQLatin1CharDecl/StringDecl
    // To avoid creating multiple fixit in case of nested QLatin1Char/String calls
    // it is important to only test the one right after a CXXFunctionalCastExpr with QLatin1Char/String name
    if (isa<CXXFunctionalCastExpr>(parent_stmt)) {
        auto *parent = dyn_cast<CXXFunctionalCastExpr>(parent_stmt);
        if (parent->getConversionFunction()->getNameAsString() != "QLatin1Char" && parent->getConversionFunction()->getNameAsString() != "QLatin1String") {
            return false;
        } // need to check that this call is related to a QString or a QChar
        if (check_parent) {
            m_QStringOrQChar_fix = relatedToQStringOrQChar(parent_stmt, context);
        }
        // in case of looking for left over, we don't do it here, because might go past the QLatin1Char/String we are nested in
        // and replace the one within
        // QString toto = QLatin1String ( something_not_supported ? QLatin1String("should not be corrected") : "toto" )
        // the inside one should not be corrected because the outside QLatin1String is staying.
        m_QChar = parent->getConversionFunction()->getNameAsString() == "QLatin1Char";

        oneFunctionalCast = true;
    }

    // Not checking the parent when looking for left over QLatin1String call nested in a QLatin1String whose fix is not supported
    if (!check_parent) {
        return oneFunctionalCast;
    }

    parent_stmt = context->parentMap->getParent(parent_stmt);
    // If an other CXXFunctionalCastExpr QLatin1String is found among the parents
    // the present QLatin1String call is nested in an other QLatin1String call and should be ignored.
    // The outer call will take care of it.
    // Unless the outer call is from a Macro, in which case the current call should not be ignored
    while (parent_stmt) {
        if (isa<CXXFunctionalCastExpr>(parent_stmt)) {
            auto *parent = dyn_cast<CXXFunctionalCastExpr>(parent_stmt);
            NamedDecl *ndecl = parent->getConversionFunction();
            if (ndecl) {
                if (ndecl->getNameAsString() == "QLatin1Char" || ndecl->getNameAsString() == "QLatin1String") {
                    if (parent_stmt->getBeginLoc().isMacroID()) {
                        auto parent_stmt_begin = parent_stmt->getBeginLoc();
                        auto parent_stmt_end = parent_stmt->getEndLoc();
                        auto parent_spl_begin = sm().getSpellingLoc(parent_stmt_begin);
                        auto parent_spl_end = sm().getSpellingLoc(parent_stmt_end);
                        auto ctorSpelling_loc = sm().getSpellingLoc(ctorExpr->getBeginLoc());
                        if (sm().isPointWithin(ctorSpelling_loc, parent_spl_begin, parent_spl_end)) {
                            return false;
                        }
                        return oneFunctionalCast;
                    }

                    return false;
                }
            }
        }
        parent_stmt = context->parentMap->getParent(parent_stmt);
    }

    return oneFunctionalCast;
}

bool Qt6QLatin1StringCharToU::warningAlreadyEmitted(SourceLocation sploc)
{
    return std::find(m_emittedWarningsInMacro.begin(), m_emittedWarningsInMacro.end(), sploc) != m_emittedWarningsInMacro.end();
}

void Qt6QLatin1StringCharToU::VisitStmt(clang::Stmt *stmt)
{
    auto *ctorExpr = dyn_cast<CXXConstructExpr>(stmt);
    if (!ctorExpr) {
        return;
    }
    m_QStringOrQChar_fix = false;
    if (!isInterestingCtorCall(ctorExpr, m_context, true)) {
        return;
    }

    std::vector<FixItHint> fixits;
    std::string message;

    for (auto macro_pos : m_listingMacroExpand) {
        if (sm().isPointWithin(macro_pos, stmt->getBeginLoc(), stmt->getEndLoc())) {
            message = "QLatin1Char or QLatin1String is being called (fix it not supported because of macro)";
            emitWarning(stmt->getBeginLoc(), message, fixits);
            return;
        }
    }
    if (!m_QStringOrQChar_fix) {
        message = "QLatin1Char or QLatin1String is being called (fix it not supported)";
        emitWarning(stmt->getBeginLoc(), message, fixits);
        return;
    }

    checkCTorExpr(stmt, true);
}

bool Qt6QLatin1StringCharToU::checkCTorExpr(clang::Stmt *stmt, bool check_parents)
{
    auto *ctorExpr = dyn_cast<CXXConstructExpr>(stmt);
    if (!ctorExpr) {
        return false;
    }

    std::vector<FixItHint> fixits;
    std::string message;

    // parents are not checked when looking inside a QLatin1Char/String that does not support fixes
    // extra parentheses might be needed for the inner QLatin1Char/String fix
    bool extra_parentheses = !check_parents;

    bool noFix = false;

    SourceLocation warningLocation = stmt->getBeginLoc();

    if (!isInterestingCtorCall(ctorExpr, m_context, check_parents)) {
        return false;
    }
    message = "QLatin1Char or QLatin1String is being called";
    if (stmt->getBeginLoc().isMacroID()) {
        SourceLocation callLoc = stmt->getBeginLoc();
        message += " in macro ";
        message += Lexer::getImmediateMacroName(callLoc, sm(), lo());
        message += ". Please replace with `u` call manually.";
        SourceLocation sploc = sm().getSpellingLoc(callLoc);
        warningLocation = sploc;
        if (warningAlreadyEmitted(sploc)) {
            return false;
        }

        m_emittedWarningsInMacro.push_back(sploc);
        // We don't support fixit within macro. (because the replacement is wrong within the #define)
        emitWarning(sploc, message, fixits);
        return true;
    }

    std::string replacement = buildReplacement(stmt, noFix, extra_parentheses);
    if (!noFix) {
        fixits.push_back(FixItHint::CreateReplacement(stmt->getSourceRange(), replacement));
    }

    emitWarning(warningLocation, message, fixits);

    if (noFix) {
        m_QChar_noFix = m_QChar; // because QLatin1Char with QLatin1Char whose fix is unsupported should be corrected
                                 // unlike QLatin1String
        lookForLeftOver(stmt, m_QChar);
    }

    return true;
}

void Qt6QLatin1StringCharToU::lookForLeftOver(clang::Stmt *stmt, bool found_QString_QChar)
{
    Stmt *current_stmt = stmt;
    bool keep_looking = true;
    // remembering the QString or QChar trace from the other sibling in case of CXXMemberCallExpr
    // in order to catch QLatin1String("notcaught") in the following example
    // s1 = QLatin1String(s2df.contains(QLatin1String("notcaught"))? QLatin1String("dontfix1") : QLatin1String("dontfix2"));
    bool remember = false;
    if (isa<CXXMemberCallExpr>(current_stmt)) {
        remember = true;
    }
    for (auto it = current_stmt->child_begin(); it != current_stmt->child_end(); it++) {
        Stmt *child = *it;

        // here need to make sure a QChar or QString type is present between the first current_stmt and the one we are testing
        // should not check the parents because we might go past the QLatin1String or QLatin1Char whose fix was not supported
        if (!found_QString_QChar) {
            found_QString_QChar = foundQCharOrQString(child);
        }

        // if no QString or QChar signature as been found, no point to check for QLatin1String or QLatin1Char to correct.
        if (found_QString_QChar) {
            keep_looking = !checkCTorExpr(child, false); // if QLatin1Char/String is found, stop looking into children of current child
        }
        // the QLatin1Char/String calls present there, if any, will be caught
        if (keep_looking) {
            lookForLeftOver(child, found_QString_QChar);
        }

        if (!remember) {
            found_QString_QChar = m_QChar_noFix;
        }
    }
}

std::string Qt6QLatin1StringCharToU::buildReplacement(clang::Stmt *stmt, bool &noFix, bool extra, bool ancestorIsCondition, int ancestorConditionChildNumber)
{
    std::string replacement;
    Stmt *current_stmt = stmt;

    int i = 0;

    for (auto it = current_stmt->child_begin(); it != current_stmt->child_end(); it++) {
        Stmt *child = *it;
        auto *parent_condOp = dyn_cast<ConditionalOperator>(current_stmt);
        auto *child_condOp = dyn_cast<ConditionalOperator>(child);

        if (parent_condOp) {
            ancestorIsCondition = true;
            ancestorConditionChildNumber = i;
            if (ancestorConditionChildNumber == 2) {
                replacement += " : ";
            }
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

        auto *child_declRefExp = dyn_cast<DeclRefExpr>(child);
        auto *child_boolLitExp = dyn_cast<CXXBoolLiteralExpr>(child);
        auto *child_charliteral = dyn_cast<CharacterLiteral>(child);
        auto *child_stringliteral = dyn_cast<StringLiteral>(child);

        if (child_stringliteral) {
            replacement += "u\"";
            replacement += child_stringliteral->getString();
            replacement += "\"";
            replacement += "_qs";
        } else if (child_charliteral) {
            replacement += "u\'";
            if (child_charliteral->getValue() == 92 || child_charliteral->getValue() == 39) {
                replacement += "\\";
            }
            replacement += child_charliteral->getValue();
            replacement += "\'";
        } else if (child_boolLitExp) {
            replacement = child_boolLitExp->getValue() ? "true" : "false";
            replacement += " ? ";
        } else if (child_declRefExp) {
            if (ancestorIsCondition && ancestorConditionChildNumber == 0 && child_declRefExp->getType().getAsString() == "_Bool") {
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

void Qt6QLatin1StringCharToU::VisitMacroExpands(const clang::Token & /*MacroNameTok*/, const clang::SourceRange &range, const MacroInfo * /*info*/)
{
    m_listingMacroExpand.push_back(range.getBegin());
    return;
}
