/*
    SPDX-FileCopyrightText: 2015 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLANG_COPYABLE_POLYMORPHIC_H
#define CLANG_COPYABLE_POLYMORPHIC_H

#include "checkbase.h"

#include <string>

/**
 * Finds polymorphic classes without Q_DISABLE_COPY
 *
 * See README-copyable-polymorphic for more information
 */
class CopyablePolymorphic : public CheckBase
{
public:
    explicit CopyablePolymorphic(const std::string &name, ClazyContext *context);
    void VisitDecl(clang::Decl *) override;
    std::vector<clang::FixItHint> fixits(clang::CXXRecordDecl *record);
};

#endif
