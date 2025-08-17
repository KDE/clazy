/*
    Copyright (C) 2025 Ahmad Samir <a.samirh78@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLAZY_QBYTEARRAY_CONVERSION_TO_C_STYLE
#define CLAZY_QBYTEARRAY_CONVERSION_TO_C_STYLE

#include "checkbase.h"

/**
 * See README-qbytearray-conversion-to-c-style.md for more info.
 */
class QBytearrayConversionToCStyle : public CheckBase
{
public:
    using CheckBase::CheckBase;
    void VisitStmt(clang::Stmt *) override;
};

#endif // CLAZY_QBYTEARRAY_CONVERSION_TO_C_STYLE
