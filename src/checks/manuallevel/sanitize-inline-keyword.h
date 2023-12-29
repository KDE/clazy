/*
    SPDX-FileCopyrightText: 2023 Ahmad Samir <a.samirh78@gmail.com>

  SPDX-License-Identifier: LGPL-2.0-or-later
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
