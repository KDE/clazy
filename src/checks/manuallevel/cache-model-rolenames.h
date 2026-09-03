/*
    SPDX-FileCopyrightText: 2026 Alexander Lohnau <alexander.lohnau@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLAZY_CACHE_MODEL_ROLENAMES_H
#define CLAZY_CACHE_MODEL_ROLENAMES_H

#include "checkbase.h"

/**
 * See README-cache-model-rolenames.md for more info.
 */
class CacheModelRolenames : public CheckBase
{
public:
    using CheckBase::CheckBase;
    void VisitDecl(clang::Decl *) override;
};

#endif
