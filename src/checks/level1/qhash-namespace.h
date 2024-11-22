/*
    SPDX-FileCopyrightText: 2017 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLAZY_QHASH_NAMESPACE_H
#define CLAZY_QHASH_NAMESPACE_H

#include "checkbase.h"

#include <string>

namespace clang
{
class Decl;
} // namespace clang

/**
 * See README-qhash-namespace.md for more info.
 */
class QHashNamespace : public CheckBase
{
public:
    explicit QHashNamespace(const std::string &name, ClazyContext *context);
    void VisitDecl(clang::Decl *decl) override;
};

#endif
