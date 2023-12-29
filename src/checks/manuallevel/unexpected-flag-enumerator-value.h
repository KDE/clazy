/*
    SPDX-FileCopyrightText: 2021 Klarälvdalens Datakonsult AB a KDAB Group company info@kdab.com
  Author: Waqar Ahmed <waqar.ahmed@kdab.com>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLAZY_UNEXPECTED_FLAG_ENUMERATOR_VALUE_H
#define CLAZY_UNEXPECTED_FLAG_ENUMERATOR_VALUE_H

#include "checkbase.h"

/**
 * See README-unexpected-flag-enumerator-value.md for more info.
 */
class UnexpectedFlagEnumeratorValue : public CheckBase
{
public:
    explicit UnexpectedFlagEnumeratorValue(const std::string &name, ClazyContext *context);
    void VisitDecl(clang::Decl *) override;

private:
};

#endif
