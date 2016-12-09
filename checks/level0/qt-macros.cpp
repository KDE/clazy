/*
  This file is part of the clazy static checker.

  Copyright (C) 2016 Sergio Martins <smartins@kde.org>

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

#include "qt-macros.h"
#include "Utils.h"
#include "HierarchyUtils.h"
#include "QtUtils.h"
#include "TypeUtils.h"
#include "checkmanager.h"

#include <clang/AST/AST.h>
#include <clang/Parse/Parser.h>

using namespace clang;
using namespace std;

#if LLVM_VERSION_MAJOR == 3 && LLVM_VERSION_MINOR > 6
class QtMacrosPreprocessorCallbacks : public clang::PPCallbacks
{
public:
    QtMacrosPreprocessorCallbacks(QtMacros *q, const SourceManager &sm, const LangOptions &lo)
        : clang::PPCallbacks()
        , q(q)
        , m_sm(sm)
        , m_langOpts(lo)
    {
    }

    void MacroDefined(const Token &MacroNameTok, const MacroDirective *MD) override
    {
        if (q->m_OSMacroExists)
            return;

        IdentifierInfo *ii = MacroNameTok.getIdentifierInfo();
        if (!ii)
            return;

        if (clazy_std::startsWith(ii->getName(), "Q_OS_"))
            q->m_OSMacroExists = true;
    }

    void checkIfDef(const Token &MacroNameTok, SourceLocation Loc)
    {
        IdentifierInfo *ii = MacroNameTok.getIdentifierInfo();
        if (!ii)
            return;

        if (ii->getName() == "Q_OS_WINDOWS") {
            q->emitWarning(Loc, "Q_OS_WINDOWS is wrong, use Q_OS_WIN instead");
        } else if (!q->m_OSMacroExists && clazy_std::startsWith(ii->getName(), "Q_OS_")) {
            q->emitWarning(Loc, "Include qglobal.h before testing Q_OS_ macros");
        }
    }

    void Defined(const Token &MacroNameTok, const MacroDefinition &MD, SourceRange Range) override
    {
        checkIfDef(MacroNameTok, Range.getBegin());
    }

    void Ifdef(SourceLocation Loc, const Token &MacroNameTok, const MacroDefinition &) override
    {
        checkIfDef(MacroNameTok, Loc);
    }

    QtMacros *const q;
    const SourceManager& m_sm;
    LangOptions m_langOpts;
};
#endif

QtMacros::QtMacros(const std::string &name, const clang::CompilerInstance &ci)
    : CheckBase(name, ci)
{
#if LLVM_VERSION_MAJOR == 3 && LLVM_VERSION_MINOR > 6
    m_preprocessorCallbacks = new QtMacrosPreprocessorCallbacks(this, sm(), lo());
    Preprocessor &pi = m_ci.getPreprocessor();
    pi.addPPCallbacks(std::unique_ptr<PPCallbacks>(m_preprocessorCallbacks));
#endif
}


REGISTER_CHECK_WITH_FLAGS("qt-macros", QtMacros, CheckLevel0)
