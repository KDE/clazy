/*
    This file is part of the clazy static checker.

    SPDX-FileCopyrightText: 2015 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLANG_LAZY_RULE_OF_TWO_SOFT_H
#define CLANG_LAZY_RULE_OF_TWO_SOFT_H

#include "checks/ruleofbase.h"

#include <string>

class ClazyContext;

namespace clang
{
class Stmt;
}

/**
 * Finds classes or structs which violate the rule of two.
 * If a class has a copy-ctor it should have copy-assignment operator too, and vice-versa.
 *
 * See README-rule-of-two-soft for more information
 */
class RuleOfTwoSoft : public RuleOfBase
{
public:
    explicit RuleOfTwoSoft(const std::string &name, ClazyContext *context);
    void VisitStmt(clang::Stmt *s) override;
};

#endif
