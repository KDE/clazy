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

#include "qenums.h"
#include "Utils.h"
#include "HierarchyUtils.h"
#include "QtUtils.h"
#include "TypeUtils.h"
#include "checkmanager.h"

#include <clang/AST/AST.h>
#include <clang/Lex/PPCallbacks.h>
#include <clang/Lex/Token.h>
#include <clang/Lex/Preprocessor.h>

using namespace clang;
using namespace std;

#if LLVM_VERSION_MAJOR == 3 && LLVM_VERSION_MINOR > 6
class QEnumsPreprocessorCallbacks : public clang::PPCallbacks
{
public:
    QEnumsPreprocessorCallbacks(const QEnumsPreprocessorCallbacks &) = delete;
    QEnumsPreprocessorCallbacks(Qenums *q, const SourceManager &sm, const LangOptions &lo)
        : clang::PPCallbacks()
        , q(q)
        , m_sm(sm)
        , m_langOpts(lo)
    {
    }

    void MacroExpands (const Token &MacroNameTok, const MacroDefinition &MD, SourceRange range, const MacroArgs *Args) override
    {
        IdentifierInfo *ii = MacroNameTok.getIdentifierInfo();
        if (!ii || ii->getName() != "Q_ENUMS")
            return;

        if (range.getBegin().isMacroID())
            return;

        if (m_sm.isInSystemHeader(range.getBegin()))
            return;

        q->emitWarning(range.getBegin(), "Use Q_ENUM instead of Q_ENUMS");
    }

    Qenums *const q;
    const SourceManager& m_sm;
    LangOptions m_langOpts;
};
#endif

Qenums::Qenums(const std::string &name, const clang::CompilerInstance &ci)
    : CheckBase(name, ci)
{
#if LLVM_VERSION_MAJOR == 3 && LLVM_VERSION_MINOR > 6
    auto callbacks = new QEnumsPreprocessorCallbacks(this, sm(), lo());
    Preprocessor &pi = m_ci.getPreprocessor();
    pi.addPPCallbacks(std::unique_ptr<PPCallbacks>(callbacks));
#endif
}

REGISTER_CHECK_WITH_FLAGS("qenums", Qenums, CheckLevel0)
