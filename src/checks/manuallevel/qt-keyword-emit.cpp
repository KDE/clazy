/*
    SPDX-FileCopyrightText: 2018 Sergio Martins <smartins@kde.org>
    SPDX-FileCopyrightText: 2023 Ahmad Samir <a.samirh78@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "qt-keyword-emit.h"
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

#include <string_view>
#include <vector>

using namespace clang;

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
