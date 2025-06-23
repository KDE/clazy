/*
    SPDX-FileCopyrightText: 2018 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLAZY_QHASH_WITH_CHAR_POINTER_KEY_H
#define CLAZY_QHASH_WITH_CHAR_POINTER_KEY_H

#include "checkbase.h"

#include <string>

/**
 * See README-qhash-with-char-pointer-key.md for more info.
 */
class QHashWithCharPointerKey : public CheckBase
{
public:
    explicit QHashWithCharPointerKey(const std::string &name);
    void VisitDecl(clang::Decl *) override;

private:
};

#endif
