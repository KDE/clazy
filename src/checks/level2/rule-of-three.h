/*
    This file is part of the clazy static checker.

    SPDX-FileCopyrightText: 2015 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLANG_LAZY_RULE_OF_THREE_H
#define CLANG_LAZY_RULE_OF_THREE_H

#include "checks/ruleofbase.h"

#include <string>

class ClazyContext;

namespace clang
{
class Decl;
}

/**
 * Finds classes or structs which violate the rule of three.
 * If a class has dtor, copy-dtor or copy-assign operator it should have all three.
 *
 * See README-rule-of-three for more information
 */
class RuleOfThree : public RuleOfBase
{
public:
    explicit RuleOfThree(const std::string &name, ClazyContext *context);
    void VisitDecl(clang::Decl *d) override;

private:
    bool shouldIgnoreType(const std::string &className) const;
};

#endif
