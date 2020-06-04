/*
  This file is part of the clazy static checker.

    Copyright (C) 2017 Sergio Martins <smartins@kde.org>

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

#include "qproperty-without-notify.h"
#include "clazy_stl.h"

#include <clang/Basic/IdentifierTable.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Lex/Lexer.h>
#include <clang/Lex/Token.h>
#include <llvm/ADT/StringRef.h>

#include <vector>

class ClazyContext;
namespace clang {
class MacroInfo;
}  // namespace clang

using namespace clang;
using namespace std;


QPropertyWithoutNotify::QPropertyWithoutNotify(const std::string &name, ClazyContext *context)
    : CheckBase(name, context, Option_CanIgnoreIncludes)
{
    enablePreProcessorCallbacks();
}

void QPropertyWithoutNotify::VisitMacroExpands(const clang::Token &MacroNameTok, const clang::SourceRange &range, const MacroInfo *)
{
    IdentifierInfo *ii = MacroNameTok.getIdentifierInfo();
    if (!ii)
        return;

    if (ii->getName() == "Q_GADGET") {
        m_lastIsGadget = true;
        return;
    }

    if (ii->getName() == "Q_OBJECT") {
        m_lastIsGadget = false;
        return;
    }

    // Gadgets can't have NOTIFY
    if (m_lastIsGadget || ii->getName() != "Q_PROPERTY")
        return;

    if (sm().isInSystemHeader(range.getBegin()))
        return;
    CharSourceRange crange = Lexer::getAsCharRange(range, sm(), lo());

    string text = static_cast<std::string>(Lexer::getSourceText(crange, sm(), lo()));
    if (text.back() == ')')
        text.pop_back();

    vector<string> split = clazy::splitString(text, ' ');

    bool found_read = false;
    bool found_constant = false;
    bool found_notify = false;
    for (std::string &token : split) {
        clazy::rtrim(/*by-ref*/token);
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

    if (!found_read || (found_notify || found_constant))
        return;


    emitWarning(range.getBegin(), "Q_PROPERTY should have either NOTIFY or CONSTANT");
}
