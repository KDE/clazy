/*
  This file is part of the clazy static checker.

    SPDX-FileCopyrightText: 2016 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLAZY_MUTABLE_CONTAINER_KEY_H
#define CLAZY_MUTABLE_CONTAINER_KEY_H

#include "checkbase.h"

#include <string>

class ClazyContext;
namespace clang
{
class Decl;
} // namespace clang

/**
 * See README-mutable-container-key for more info.
 */
class MutableContainerKey : public CheckBase
{
public:
    explicit MutableContainerKey(const std::string &name, ClazyContext *context);
    void VisitDecl(clang::Decl *decl) override;

private:
};

#endif
