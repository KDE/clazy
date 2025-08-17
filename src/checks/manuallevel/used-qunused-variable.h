/*
    SPDX-FileCopyrightText: 2024 Alexander Lohnau <alexander.lohnau@gmx.de>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "checkbase.h"

/**
 * See README-used-qunused-variable.md for more info.
 */
class UsedQUnusedVariable : public CheckBase
{
public:
    using CheckBase::CheckBase;
    void VisitDecl(clang::Decl *) override;
};
