/*
  This file is part of the clazy static checker.

    SPDX-FileCopyrightText: 2018 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLAZY_QREQUIREDRESULT_CANDIDATES_H
#define CLAZY_QREQUIREDRESULT_CANDIDATES_H

#include "checkbase.h"

/**
 * See README-qrequiredresult-candidates.md for more info.
 */
class QRequiredResultCandidates : public CheckBase
{
public:
    explicit QRequiredResultCandidates(const std::string &name, ClazyContext *context);
    void VisitDecl(clang::Decl *) override;

private:
};

#endif
