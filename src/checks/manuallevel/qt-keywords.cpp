/*
  This file is part of the clazy static checker.

    Copyright (C) 2018 Sergio Martins <smartins@kde.org>

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

#include "qt-keywords.h"
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

using namespace clang;
using namespace std;

QtKeywords::QtKeywords(const std::string &name, ClazyContext *context)
    : CheckBase(name, context)
{
    enablePreProcessorCallbacks();
    context->enablePreprocessorVisitor();
}

void QtKeywords::VisitMacroExpands(const Token &macroNameTok, const SourceRange &range, const clang::MacroInfo *minfo)
{
    IdentifierInfo *ii = macroNameTok.getIdentifierInfo();
    if (!ii || !minfo)
        return;

    if (auto ppvisitor = m_context->preprocessorVisitor) {
        // Save some CPU cycles. No point in running if QT_NO_KEYWORDS
        if (ppvisitor->isQT_NO_KEYWORDS())
            return;
    }

    static const vector<StringRef> keywords = { "foreach", "signals", "slots", "emit" };
    std::string name = static_cast<std::string>(ii->getName());
    if (!clazy::contains(keywords, name))
        return;

    // Make sure the macro is Qt's. It must be defined in Qt's headers, not 3rdparty
    std::string qtheader = static_cast<std::string>(sm().getFilename(sm().getSpellingLoc(minfo->getDefinitionLoc())));
    if (!clazy::endsWith(qtheader, "qglobal.h") && !clazy::endsWith(qtheader, "qobjectdefs.h"))
        return;

    std::vector<FixItHint> fixits;
    std::string replacement = "Q_" + name;
    std::transform(replacement.begin(), replacement.end(), replacement.begin(), ::toupper);
    fixits.push_back(clazy::createReplacement(range, replacement));

    emitWarning(range.getBegin(), "Using a Qt keyword (" + string(ii->getName()) + ")", fixits);
}
