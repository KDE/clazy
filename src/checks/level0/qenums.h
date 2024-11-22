/*
    SPDX-FileCopyrightText: 2016 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLAZY_QENUMS_H
#define CLAZY_QENUMS_H

#include "checkbase.h"

#include <clang/Basic/SourceLocation.h>

#include <string>

/**
 * See README-qenums for more info.
 */
class QEnums : public CheckBase
{
public:
    explicit QEnums(const std::string &name, ClazyContext *context);

private:
    void VisitMacroExpands(const clang::Token &MacroNameTok, const clang::SourceRange &range, const clang::MacroInfo * = nullptr) override;
};

#endif
