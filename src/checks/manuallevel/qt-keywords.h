/*
    SPDX-FileCopyrightText: 2018 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLAZY_QT_KEYWORDS_H
#define CLAZY_QT_KEYWORDS_H

#include "checkbase.h"

#include <clang/Basic/SourceLocation.h>

#include <string>

namespace clang
{
class MacroInfo;
class Token;
} // namespace clang

/**
 * See README-qt-keywords.md for more info.
 */
class QtKeywords : public CheckBase
{
public:
    explicit QtKeywords(const std::string &name, ClazyContext *context);

protected:
    void VisitMacroExpands(const clang::Token &, const clang::SourceRange &, const clang::MacroInfo *) override;
};

#endif
