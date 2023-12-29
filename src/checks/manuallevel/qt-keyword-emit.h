/*
    SPDX-FileCopyrightText: 2018 Sergio Martins <smartins@kde.org>
  SPDX-FileCopyrightText: 2023 Ahmad Samir <a.samirh78@gmail.com>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLAZY_QT_KEYWORD_EMIT_H
#define CLAZY_QT_KEYWORD_EMIT_H

#include "checkbase.h"

#include <clang/Basic/SourceLocation.h>

#include <string>

/**
 * See README-qt-keyword-emit.md for more info.
 */

class ClazyContext;
namespace clang
{
class MacroInfo;
class Token;
} // namespace clang

class QtKeywordEmit : public CheckBase
{
public:
    explicit QtKeywordEmit(const std::string &name, ClazyContext *context);

protected:
    void VisitMacroExpands(const clang::Token &, const clang::SourceRange &, const clang::MacroInfo *) override;
};

#endif // CLAZY_QT_KEYWORD_EMIT_H
