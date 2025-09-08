/*
    SPDX-FileCopyrightText: 2016 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLAZY_CTOR_MISSING_PARENT_ARGUMENT_H
#define CLAZY_CTOR_MISSING_PARENT_ARGUMENT_H

#include "checkbase.h"

/**
 * See README-ctor-missing-parent-argument for more info.
 */
class CtorMissingParentArgument : public CheckBase
{
public:
    using CheckBase::CheckBase;
    void VisitDecl(clang::Decl *decl) override;

private:
    std::string expectedParentTypeFor(clang::CXXRecordDecl *decl);
};

#endif
