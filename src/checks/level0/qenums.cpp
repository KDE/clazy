/*
    SPDX-FileCopyrightText: 2016 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "qenums.h"
#include "ClazyContext.h"
#include "PreProcessorVisitor.h"
#include "clazy_stl.h"

#include <clang/Basic/IdentifierTable.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Lex/Lexer.h>
#include <clang/Lex/Token.h>
#include <llvm/ADT/StringRef.h>

using namespace clang;

QEnums::QEnums(const std::string &name, ClazyContext *context)
    : CheckBase(name, context)
{
}

void QEnums::VisitMacroExpands(const Token &MacroNameTok, const SourceRange &range, const clang::MacroInfo *)
{
    const PreProcessorVisitor *preProcessorVisitor = m_context->preprocessorVisitor;
    if (!preProcessorVisitor || preProcessorVisitor->qtVersion() < 50500) {
        return;
    }

    IdentifierInfo *ii = MacroNameTok.getIdentifierInfo();
    if (!ii || ii->getName() != "Q_ENUMS") {
        return;
    }

    {
        // Don't warn when importing enums of other classes, because Q_ENUM doesn't support it.
        // We simply check if :: is present because it's very cumbersome to to check for different classes when dealing with the pre-processor

        CharSourceRange crange = Lexer::getAsCharRange(range, sm(), lo());
        std::string text = static_cast<std::string>(Lexer::getSourceText(crange, sm(), lo()));
        if (clazy::contains(text, "::")) {
            return;
        }
    }

    if (range.getBegin().isMacroID()) {
        return;
    }

    if (sm().isInSystemHeader(range.getBegin())) {
        return;
    }

    emitWarning(range.getBegin(), "Use Q_ENUM instead of Q_ENUMS");
}
