/*
    SPDX-FileCopyrightText: 2018 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "qt-keywords.h"
#include "ClazyContext.h"
#include "FixItUtils.h"
#include "PreProcessorVisitor.h"
#include "clazy_stl.h"

#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/IdentifierTable.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Lex/MacroInfo.h>
#include <clang/Lex/Token.h>
#include <llvm/ADT/StringRef.h>

#include <algorithm>
#include <ctype.h>
#include <vector>

using namespace clang;

QtKeywords::QtKeywords(const std::string &name, ClazyContext *context)
    : CheckBase(name, context)
{
    enablePreProcessorCallbacks();
    context->enablePreprocessorVisitor();
}

void QtKeywords::VisitMacroExpands(const Token &macroNameTok, const SourceRange &range, const clang::MacroInfo *minfo)
{
    IdentifierInfo *ii = macroNameTok.getIdentifierInfo();
    if (!ii || !minfo) {
        return;
    }

    if (auto *ppvisitor = m_context->preprocessorVisitor) {
        // Save some CPU cycles. No point in running if QT_NO_KEYWORDS
        if (ppvisitor->isQT_NO_KEYWORDS()) {
            return;
        }
    }

    static const std::vector<StringRef> keywords = {"foreach", "signals", "slots", "emit"};
    std::string name = static_cast<std::string>(ii->getName());
    if (!clazy::contains(keywords, name)) {
        return;
    }

    // Make sure the macro is Qt's. It must be defined in Qt's headers, not 3rdparty
    std::string qtheader = static_cast<std::string>(sm().getFilename(sm().getSpellingLoc(minfo->getDefinitionLoc())));
    if (!clazy::endsWithAny(qtheader, {"qglobal.h", "qobjectdefs.h", "qtmetamacros.h", "qforeach.h"})) {
        return;
    }

    std::vector<FixItHint> fixits;
    std::string replacement = "Q_" + name;
    std::transform(replacement.begin(), replacement.end(), replacement.begin(), ::toupper);
    fixits.push_back(clazy::createReplacement(range, replacement));

    emitWarning(range.getBegin(), "Using a Qt keyword (" + std::string(ii->getName()) + ")", fixits);
}
