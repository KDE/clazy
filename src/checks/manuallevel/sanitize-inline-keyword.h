/*
  This file is part of the clazy static checker.

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

#ifndef CLAZY_SANITIZE_INLINE_KEYWORD_H
#define CLAZY_SANITIZE_INLINE_KEYWORD_H

#include "checkbase.h"

/**
 * Emits a warning if the "inline" keyword is set on the definition
 * but not the declaration of a member method in an exported class.
 *
 * This check sets Option_CanIgnoreIncludes.
 *
 * See docs/checks/README-sanitize-inline-keyword.md for more info.
 */
class SanitizeInlineKeyword : public CheckBase
{
public:
    explicit SanitizeInlineKeyword(const std::string &name, ClazyContext *context);

    void VisitDecl(clang::Decl *) override;
};

#endif
