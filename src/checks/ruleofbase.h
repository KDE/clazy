/*
    SPDX-FileCopyrightText: 2015 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLANG_LAZY_RULE_OF_BASE_H
#define CLANG_LAZY_RULE_OF_BASE_H

#include "checkbase.h"

#include <string>

namespace clang
{
class CXXRecordDecl;
}

/**
 * Base class for RuleOfTwo and RuleOfThree
 */
class RuleOfBase : public CheckBase
{
public:
    explicit RuleOfBase(const std::string &name, Options options);

protected:
    bool isBlacklisted(clang::CXXRecordDecl *record) const;
};

#endif
