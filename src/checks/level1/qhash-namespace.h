/*
    SPDX-FileCopyrightText: 2017 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLAZY_QHASH_NAMESPACE_H
#define CLAZY_QHASH_NAMESPACE_H

#include "checkbase.h"

/**
 * See README-qhash-namespace.md for more info.
 */
class QHashNamespace : public CheckBase
{
public:
    using CheckBase::CheckBase;
    void VisitDecl(clang::Decl *decl) override;

private:
    std::string fullyQualifiedParamName(clang::ParmVarDecl *firstArg);
};

#endif
