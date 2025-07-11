/*
    SPDX-FileCopyrightText: 2018 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLAZY_QT_KEYWORDS_H
#define CLAZY_QT_KEYWORDS_H

#include "checkbase.h"

#include <string>

/**
 * See README-qt-keywords.md for more info.
 */
class QtKeywords : public CheckBase
{
public:
    explicit QtKeywords(const std::string &name);

protected:
    void VisitMacroExpands(const clang::Token &, const clang::SourceRange &, const clang::MacroInfo *) override;
};

#endif
