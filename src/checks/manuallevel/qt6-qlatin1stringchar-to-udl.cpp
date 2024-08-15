/*
    SPDX-FileCopyrightText: 2020 The Qt Company Ltd.
    SPDX-FileCopyrightText: 2020 Lucie Gerard <lucie.gerard@qt.io>
    SPDX-FileCopyrightText: 2024 Ahmad Samir <a.samirh78@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "qt6-qlatin1stringchar-to-udl.h"
#include "ClazyContext.h"
#include "ContextUtils.h"
#include "FixItUtils.h"
#include "HierarchyUtils.h"
#include "PreProcessorVisitor.h"
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

Qt6QLatin1StringCharToUdl::Qt6QLatin1StringCharToUdl(const std::string &name, ClazyContext *context)
    : CheckBase(name, context, Option_CanIgnoreIncludes)
{
    enablePreProcessorCallbacks();
    context->enablePreprocessorVisitor();
}

static bool isQLatin1CharDecl(CXXConstructorDecl *decl)
{
    return decl && clazy::isOfClass(decl, "QLatin1Char");
}

static bool isQLatin1StringDecl(CXXConstructorDecl *decl)
{
    if (!decl)
        return false;
    auto name = decl->getNameAsString();
    return name == "QLatin1StringView" || name == "QLatin1String";
}

static bool isQLatin1ClassName(StringRef str)
{
    return str == "QLatin1StringView" || str == "QLatin1String" || str == "QLatin1Char";
}

static bool canReplaceQLatin1Char(Stmt *stmt)
{
    if (auto *expr = dyn_cast<CallExpr>(stmt)) {
        // Don't replace with u'' if it's a function parameter: `void f(QLatin1Char c)`
        for (const auto arg : expr->arguments()) {
            if (arg->getType().getAsString() == "QLatin1Char") {
                return false;
            }
        }
    } else if (auto *decl = dyn_cast<DeclStmt>(stmt)) {
        // Don't replace with u'' if assigning to a QLatin1Char: `QLatin1Char ch = QLatin1Char('a')`
        for (const auto d : decl->decls()) {
            if (auto *s = dyn_cast<VarDecl>(d); s && s->getType().getAsString() == "QLatin1Char") {
                return false;
            }
        }
    }
    return true;
}

/*
 * To be interesting, the CXXContructExpr:
 * 1/ must be of class QLatin1String{,View} or QLatin1Char
 * 2/ must have a CXXFunctionalCastExpr with name QLatin1String{,View} or QLatin1Char
 *    (to pick only one of two the CXXContructExpr of class QLatin1String)
 * 3/ must not be nested within another QLatin1String{,View}/QLatin1Char call (unless
 *    looking for left over).
 *    This is done by looking for a CXXFunctionalCastExpr with the class name among
 *    parents. These nested calls are processed while visiting the outermost call.
 *
 * Don't replace QLatin1Char in a couple of special cases (see canReplaceQLatin1Char())
 */
std::optional<std::string> Qt6QLatin1StringCharToUdl::isInterestingCtorCall(CXXConstructExpr *ctorExpr, const ClazyContext *const context, bool check_parent)
{
    CXXConstructorDecl *ctorDecl = ctorExpr->getConstructor();
    if (!isQLatin1CharDecl(ctorDecl) && !isQLatin1StringDecl(ctorDecl)) {
        return {};
    }

    Stmt *parent_stmt = clazy::parent(context->parentMap, ctorExpr);
    if (!parent_stmt) {
        return {};
    }
    bool oneFunctionalCast = false;
    std::string ctorName;
    // A given QLatin1Char/String call will have two ctorExpr passing the isQLatin1CharDecl/StringDecl
    // To avoid creating multiple fixit in case of nested QLatin1Char/String calls
    // it is important to only test the one right after a CXXFunctionalCastExpr with QLatin1Char/String name
    if (auto *parent = dyn_cast<CXXFunctionalCastExpr>(parent_stmt)) {
        ctorName = parent->getConversionFunction()->getNameAsString();
        if (!isQLatin1ClassName(ctorName)) {
            return {};
        }

        oneFunctionalCast = true;
    }

    // Not checking the parent when looking for left over QLatin1String call nested in a QLatin1String whose fix is not supported
    if (!check_parent) {
        return oneFunctionalCast ? std::optional{ctorName} : std::nullopt;
    }

    // If an other CXXFunctionalCastExpr QLatin1String is found among the parents
    // the present QLatin1String call is nested in an other QLatin1String call and should be ignored.
    // The outer call will take care of it.
    // Unless the outer call is from a Macro, in which case the current call should not be ignored
    while ((parent_stmt = context->parentMap->getParent(parent_stmt))) {
        if (auto *parent = dyn_cast<CXXFunctionalCastExpr>(parent_stmt)) {
            if (NamedDecl *ndecl = parent->getConversionFunction()) {
                const auto name = ndecl->getNameAsString();
                if (isQLatin1ClassName(name)) {
                    if (parent_stmt->getBeginLoc().isMacroID()) {
                        auto parent_stmt_begin = parent_stmt->getBeginLoc();
                        auto parent_stmt_end = parent_stmt->getEndLoc();
                        auto parent_spl_begin = sm().getSpellingLoc(parent_stmt_begin);
                        auto parent_spl_end = sm().getSpellingLoc(parent_stmt_end);
                        auto ctorSpelling_loc = sm().getSpellingLoc(ctorExpr->getBeginLoc());
                        if (m_sm.isPointWithin(ctorSpelling_loc, parent_spl_begin, parent_spl_end)) {
                            return {};
                        }
                        return oneFunctionalCast ? std::optional{name} : std::nullopt;;
                    }

                    return {};
                }
            }
        } else if (!canReplaceQLatin1Char(parent_stmt)) {
            return {};
        }
    }

    return oneFunctionalCast ? std::optional{ctorName} : std::nullopt;
}

bool Qt6QLatin1StringCharToUdl::warningAlreadyEmitted(SourceLocation sploc)
{
    return std::find(m_emittedWarningsInMacro.begin(), m_emittedWarningsInMacro.end(), sploc) != m_emittedWarningsInMacro.end();
}

void Qt6QLatin1StringCharToUdl::VisitStmt(clang::Stmt *stmt)
{
    auto *ctorExpr = dyn_cast<CXXConstructExpr>(stmt);
    if (!ctorExpr) {
        return;
    }

    auto ctorName = isInterestingCtorCall(ctorExpr, m_context, true);
    if (!ctorName) {
        return;
    }

    std::vector<FixItHint> fixits;
    std::string message;

    for (auto macro_pos : m_listingMacroExpand) {
        if (m_sm.isPointWithin(macro_pos, stmt->getBeginLoc(), stmt->getEndLoc())) {
            message = *ctorName + " is being called (fixit not supported because of macro)";
            emitWarning(stmt->getBeginLoc(), message, fixits);
            return;
        }
    }

    checkCTorExpr(stmt, true);
}

static bool checkFileExtension(const std::string &filename)
{
    // https://gcc.gnu.org/onlinedocs/gcc-14.2.0/gcc/Overall-Options.html#index-file-name-suffix-71
    using namespace std::string_view_literals;
    constexpr std::array<std::string_view, 7> extensions = {
        ".cc",
        ".cp",
        ".cxx",
        ".cpp",
        ".CPP",
        ".c++",
        ".C",
    };

    auto fileEndsWith = [&filename](std::string_view ext) {
        return clazy::endsWith(filename, ext);
    };
    return std::find_if(extensions.begin(), extensions.end(), fileEndsWith) != extensions.end();
}

void Qt6QLatin1StringCharToUdl::insertUsingNamespace(Stmt *stmt)
{
    // "using namespace Qt::StringLiterals" already exists, or has been added
    if (m_has_using_namespace)
        return;

    // Only in .cpp files
    if (!checkFileExtension(Utils::filenameForLoc(stmt->getBeginLoc(), m_sm))) {
        return;
    }

    static const std::string qtStringLiterals = "using namespace Qt::StringLiterals";
    DeclContext *declContext = m_context->lastDecl->getDeclContext();
    auto visibleContexts = clazy::contextsForDecl(declContext);
    for (DeclContext *context : visibleContexts) {
        for (const auto *udir : context->using_directives()) {
            if (getTextFromRange(udir->getSourceRange()) == qtStringLiterals) {
                m_has_using_namespace = true;
                return;
            }
        }
    }

    if (!m_has_using_namespace) { // Doesn't exist, add it
        std::vector<FixItHint> fixits;
        // "\n\n" because endOfIncludeSection() doesn't end with a new line
        fixits.push_back(clazy::createInsertion(m_context->preprocessorVisitor->endOfIncludeSection(),
                                                "\n\n" + qtStringLiterals + "\n"));
        emitWarning(stmt->getBeginLoc(), "Inserting \"using namespace Qt::StringLiterals\"", fixits);
        m_has_using_namespace = true;
    }
}

bool Qt6QLatin1StringCharToUdl::checkCTorExpr(clang::Stmt *stmt, bool check_parents)
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

    auto ctorName = isInterestingCtorCall(ctorExpr, m_context, check_parents);
    if (!ctorName) {
        return false;
    }
    const std::string &name = *ctorName;
    message = name + " is being called";
    if (stmt->getBeginLoc().isMacroID()) {
        SourceLocation callLoc = stmt->getBeginLoc();
        message += " in macro ";
        message += Lexer::getImmediateMacroName(callLoc, m_sm, lo());
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

    // At least one QLatin1String fixit, before inserting "using namespace Qt::StringLiterals"
    if (!fixits.empty() && (name == "QLatin1StringView" || name == "QLatin1String")) {
        insertUsingNamespace(stmt);
    }

    emitWarning(warningLocation, message, fixits);

    if (noFix) {
        lookForLeftOver(stmt);
    }

    return true;
}

void Qt6QLatin1StringCharToUdl::lookForLeftOver(clang::Stmt *stmt)
{
    Stmt *current_stmt = stmt;

    for (auto it = current_stmt->child_begin(); it != current_stmt->child_end(); it++) {
        Stmt *child = *it;
        bool keep_looking = !checkCTorExpr(child, false); // if QLatin1Char/String is found, stop looking into children of current child
        // the QLatin1Char/String calls present there, if any, will be caught
        if (keep_looking) {
            lookForLeftOver(child);
        }
    }
}

std::string Qt6QLatin1StringCharToUdl::buildReplacement(clang::Stmt *stmt, bool &noFix, bool extra, bool ancestorIsCondition, int ancestorConditionChildNumber)
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
            replacement += "\"";
            replacement += child_stringliteral->getString();
            replacement += "\"";
            replacement += "_L1";
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

void Qt6QLatin1StringCharToUdl::VisitMacroExpands(const clang::Token & /*MacroNameTok*/, const clang::SourceRange &range, const MacroInfo * /*info*/)
{
    m_listingMacroExpand.push_back(range.getBegin());
    return;
}
