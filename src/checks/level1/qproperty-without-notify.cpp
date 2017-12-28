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
#include "Utils.h"
#include "HierarchyUtils.h"
#include "QtUtils.h"
#include "TypeUtils.h"

#include <clang/AST/AST.h>

using namespace clang;
using namespace std;


QPropertyWithoutNotify::QPropertyWithoutNotify(const std::string &name, ClazyContext *context)
    : CheckBase(name, context, Option_CanIgnoreIncludes)
{
    enablePreProcessorCallbacks();
}

void QPropertyWithoutNotify::VisitMacroExpands(const clang::Token &MacroNameTok, const clang::SourceRange &range)
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

    if (m_lastIsGadget || ii->getName() != "Q_PROPERTY")
        return;

    if (sm().isInSystemHeader(range.getBegin()))
        return;
    CharSourceRange crange = Lexer::getAsCharRange(range, sm(), lo());

    string text = Lexer::getSourceText(crange, sm(), lo());
    vector<string> split = clazy::splitString(text, ' ');

    bool found_read = false;
    bool found_constant = false;
    bool found_notify = false;
    for (const std::string &token : split) {
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
