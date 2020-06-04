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
#include "ClazyContext.h"
#include "PreProcessorVisitor.h"
#include "clazy_stl.h"

#include <clang/Lex/Token.h>
#include <clang/Basic/IdentifierTable.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Lex/Lexer.h>
#include <llvm/ADT/StringRef.h>

namespace clang {
class MacroInfo;
}  // namespace clang

using namespace clang;
using namespace std;

QEnums::QEnums(const std::string &name, ClazyContext *context)
    : CheckBase(name, context)
{
    enablePreProcessorCallbacks();
    context->enablePreprocessorVisitor();
}

void QEnums::VisitMacroExpands(const Token &MacroNameTok, const SourceRange &range, const clang::MacroInfo *)
{
    PreProcessorVisitor *preProcessorVisitor = m_context->preprocessorVisitor;
    if (!preProcessorVisitor || preProcessorVisitor->qtVersion() < 50500)
        return;

    IdentifierInfo *ii = MacroNameTok.getIdentifierInfo();
    if (!ii || ii->getName() != "Q_ENUMS")
        return;

    {
        // Don't warn when importing enums of other classes, because Q_ENUM doesn't support it.
        // We simply check if :: is present because it's very cumbersome to to check for different classes when dealing with the pre-processor

        CharSourceRange crange = Lexer::getAsCharRange(range, sm(), lo());
        string text = static_cast<string>(Lexer::getSourceText(crange, sm(), lo()));
        if (clazy::contains(text, "::"))
            return;
    }

    if (range.getBegin().isMacroID())
        return;

    if (sm().isInSystemHeader(range.getBegin()))
        return;

    emitWarning(range.getBegin(), "Use Q_ENUM instead of Q_ENUMS");
}
