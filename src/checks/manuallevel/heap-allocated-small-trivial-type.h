/*
    SPDX-FileCopyrightText: 2019 Sergio Martins <smartins@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLAZY_HEAP_ALLOCATED_SMALL_TRIVIAL_TYPE_H
#define CLAZY_HEAP_ALLOCATED_SMALL_TRIVIAL_TYPE_H

#include "checkbase.h"

/**
 * See README-heap-allocated-small-trivial-type.md for more info.
 */
class HeapAllocatedSmallTrivialType : public CheckBase
{
public:
    explicit HeapAllocatedSmallTrivialType(const std::string &name, ClazyContext *context);
    void VisitDecl(clang::Decl *decl) override;

private:
};

#endif
