/*
  This file is part of the clazy static checker.

    Copyright (C) 2018 Sergio Martins <smartins@kde.org>
    Copyright (C) 2023 Ahmad Samir <a.samirh78@gmail.com>

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

#include "qt-keyword-emit.h"
#include "FixItUtils.h"
#include "ClazyContext.h"
#include "PreProcessorVisitor.h"
#include "clazy_stl.h"

#include <clang/Lex/MacroInfo.h>
#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/IdentifierTable.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Lex/Token.h>
#include <llvm/ADT/StringRef.h>

#include <ctype.h>
#include <algorithm>
#include <vector>
#include <string_view>

using namespace clang;

QtKeywordEmit::QtKeywordEmit(const std::string &name, ClazyContext *context)
    : CheckBase(name, context)
{
    enablePreProcessorCallbacks();
    context->enablePreprocessorVisitor();
}

void QtKeywordEmit::VisitMacroExpands(const Token &macroNameTok, const SourceRange &range, const clang::MacroInfo *minfo)
{
    IdentifierInfo *ii = macroNameTok.getIdentifierInfo();
    if (!ii || !minfo)
        return;

    if (auto ppvisitor = m_context->preprocessorVisitor) {
        // Save some CPU cycles. No point in running if QT_NO_KEYWORDS
        if (ppvisitor->isQT_NO_KEYWORDS())
            return;
    }

    static const std::string emitKeyword{"emit"};
    if (emitKeyword != std::string_view{ii->getName()})
        return;

    // Make sure the macro is Qt's. It must be defined in Qt's headers, not 3rdparty
    const std::string qtheader{sm().getFilename(sm().getSpellingLoc(minfo->getDefinitionLoc()))};
    // qobjectdefs.h in Qt5 or qtmetamacros.h in Qt6
    if (!clazy::endsWithAny(qtheader, {"qobjectdefs.h", "qtmetamacros.h"}))
        return;

    std::vector<FixItHint> fixits{clazy::createReplacement(range, "Q_EMIT")};
    emitWarning(range.getBegin(), "Using Qt (" + emitKeyword + ") keyword", fixits);
}
