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

#include "PreProcessorVisitor.h"

#if !defined(IS_OLD_CLANG)

#include <clang/Frontend/CompilerInstance.h>
#include <clang/Lex/Preprocessor.h>
#include <clang/Lex/MacroInfo.h>

using namespace clang;
using namespace std;

PreProcessorVisitor::PreProcessorVisitor(const clang::CompilerInstance &ci)
    : clang::PPCallbacks()
    , m_ci(ci)
{
    Preprocessor &pi = m_ci.getPreprocessor();
    pi.addPPCallbacks(std::unique_ptr<PPCallbacks>(this));
}

std::string PreProcessorVisitor::getTokenSpelling(const MacroDefinition &def) const
{
    if (!def)
        return {};

    MacroInfo *info = def.getMacroInfo();
    if (!info)
        return {};

    const Preprocessor &pp = m_ci.getPreprocessor();
    string result;
    for (const auto &tok : info->tokens())
        result += pp.getSpelling(tok);

    return result;
}

void PreProcessorVisitor::updateQtVersion()
{
    if (m_qtMajorVersion == -1 || m_qtPatchVersion == -1 || m_qtMinorVersion == -1) {
        m_qtVersion = -1;
    } else {
        m_qtVersion = m_qtPatchVersion + m_qtMinorVersion * 100 + m_qtMajorVersion * 10000;
    }
}

static int stringToNumber(const string &str)
{
    if (str.empty())
        return -1;

    return atoi(str.c_str());
}

void PreProcessorVisitor::MacroExpands(const Token &MacroNameTok, const MacroDefinition &def,
                                       SourceRange, const MacroArgs *)
{
    IdentifierInfo *ii = MacroNameTok.getIdentifierInfo();
    if (!ii)
        return;

    auto name = ii->getName();
    if (name == "QT_VERSION_MAJOR") {
        m_qtMajorVersion = stringToNumber(getTokenSpelling(def));
        updateQtVersion();
    }

    if (name == "QT_VERSION_MINOR") {
        m_qtMinorVersion = stringToNumber(getTokenSpelling(def));
        updateQtVersion();
    }

    if (name == "QT_VERSION_PATCH") {
        m_qtPatchVersion = stringToNumber(getTokenSpelling(def));
        updateQtVersion();
    }
}

#endif
