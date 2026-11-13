/*
    SPDX-FileCopyrightText: 2026 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLAZY_QDEBUG_PRINTF_STYLE_H
#define CLAZY_QDEBUG_PRINTF_STYLE_H

#include "checkbase.h"

/**
 * See README-qdebug-printf-style.md for more info.
 */
class QDebugPrintfStyle : public CheckBase
{
public:
    explicit QDebugPrintfStyle(const std::string &name, Options options);
    void VisitStmt(clang::Stmt *) override;
};

#endif
