/*
    SPDX-FileCopyrightText: 2023 Ahmad Samir <a.samirh78@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "sanitize-inline-keyword.h"

#include "ClazyContext.h"
#include "FixItUtils.h"
#include "QtUtils.h"
#include "TypeUtils.h"
#include "Utils.h"

#include <clang/AST/AST.h>
#include <clang/Lex/Token.h>

using namespace std::string_literals;
using namespace clang;

void SanitizeInlineKeyword::VisitDecl(Decl *decl)
{
    auto *member = dyn_cast<CXXMethodDecl>(decl);
    if (!member) {
        return;
    }

    // The class this method belongs to
    auto *parentDecl = member->getParent();
    // Only exported classes
    if (!parentDecl || parentDecl->getVisibility() == clang::HiddenVisibility) {
        return;
    }

    // constexpr methods are implicitly inline
    if (member->isConstexpr()) {
        return;
    }
    // Function templates are implicitly inline
    if (member->isTemplateDecl()) {
        return;
    }

    // Is this CXXMethodDecl* inside the class body?
    if (member->isOutOfLine()) {
        return;
    }

    // Declared/defined in-class
    if (member->isThisDeclarationADefinition()) {
        return;
    }

    FunctionDecl *definition = member->getDefinition();
    if (!definition)
        return;

    auto name = clazy::name(definition);

    auto *cxxDefinition = dyn_cast<CXXMethodDecl>(definition);
    if (!cxxDefinition) {
        return;
    }

    if (name.empty()) {
        name = clazy::name(cxxDefinition); // E.g. operator[]
    }

    if (name.empty()) {
        return; // Can't emit a warning without a method name
    }

    auto defHasInline = [](auto def) {
        return def->isInlineSpecified() && def->isThisDeclarationADefinition() && def->isOutOfLine();
    };

    if (!member->isInlineSpecified() && defHasInline(cxxDefinition)) {
        std::string msg = std::string(name) + "(): "
            + "the 'inline' keyword is specified on the definition, but not the declaration. "
              "This could lead to hard-to-suppress warnings with some compilers (e.g. MinGW). "
              "The 'inline' keyword should be used for the declaration only.";

        SourceLocation loc = member->getBeginLoc();
        std::vector<FixItHint> fixits{clazy::createInsertion(loc, "inline "s)};

        SourceLocation def = cxxDefinition->getBeginLoc();
        SourceLocation defEnd = cxxDefinition->getEndLoc();
        Token tok;
        for (; def.isValid() && def != defEnd; def = Utils::locForNextToken(def, sm(), lo())) {
            if (!Lexer::getRawToken(def, tok, sm(), lo())) { // false means success!
                if (tok.is(tok::raw_identifier) && tok.getRawIdentifier() == "inline"s) {
                    // Remove 'inline' from the definition
                    fixits.emplace_back(clazy::createReplacement(def, std::string()));
                    break;
                }
            }
        }

        emitWarning(loc, msg, fixits);
    }
}
