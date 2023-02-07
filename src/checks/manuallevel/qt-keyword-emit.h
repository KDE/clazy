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
