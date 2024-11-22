/*
    SPDX-FileCopyrightText: 2017 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "qproperty-without-notify.h"
#include "clazy_stl.h"

#include <clang/Basic/IdentifierTable.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Lex/Lexer.h>
#include <clang/Lex/Token.h>
#include <llvm/ADT/StringRef.h>

#include <vector>

using namespace clang;

QPropertyWithoutNotify::QPropertyWithoutNotify(const std::string &name, ClazyContext *context)
    : CheckBase(name, context, Option_CanIgnoreIncludes)
{
    enablePreProcessorCallbacks();
}

void QPropertyWithoutNotify::VisitMacroExpands(const clang::Token &MacroNameTok, const clang::SourceRange &range, const MacroInfo *)
{
    IdentifierInfo *ii = MacroNameTok.getIdentifierInfo();
    if (!ii) {
        return;
    }

    if (ii->getName() == "Q_GADGET") {
        m_lastIsGadget = true;
        return;
    }

    if (ii->getName() == "Q_OBJECT") {
        m_lastIsGadget = false;
        return;
    }

    // Gadgets can't have NOTIFY
    if (m_lastIsGadget || ii->getName() != "Q_PROPERTY") {
        return;
    }

    if (sm().isInSystemHeader(range.getBegin())) {
        return;
    }
    CharSourceRange crange = Lexer::getAsCharRange(range, sm(), lo());

    std::string text = static_cast<std::string>(Lexer::getSourceText(crange, sm(), lo()));
    if (text.empty()) {
        // If the text is empty, it is more likely there is an error
        // in parsing than an empty Q_PROPERTY macro call (which would
        // be a moc error anyhow).
        return;
    }

    if (text.back() == ')') {
        text.pop_back();
    }

    std::vector<std::string> split = clazy::splitString(text, ' ');

    bool found_read = false;
    bool found_constant = false;
    bool found_notify = false;
    for (std::string &token : split) {
        clazy::rtrim(/*by-ref*/ token);
        if (!found_read && token == "READ") {
            found_read = true;
            continue;
        }

        if (!found_constant && token == "CONSTANT") {
            found_constant = true;
            continue;
        }

        if (!found_notify && token == "NOTIFY") {
            found_notify = true;
            continue;
        }
    }

    if (!found_read || (found_notify || found_constant)) {
        return;
    }

    emitWarning(range.getBegin(), "Q_PROPERTY should have either NOTIFY or CONSTANT");
}
