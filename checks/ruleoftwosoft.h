/*
   This file is part of the clang-lazy static checker.

  Copyright (C) 2015 Sergio Martins <smartins@kde.org>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

  As a special exception, permission is given to link this program
  with any edition of Qt, and distribute the resulting executable,
  without including the source code for Qt in the source distribution.
*/

#ifndef CLANG_LAZY_RULE_OF_TWO_SOFT_H
#define CLANG_LAZY_RULE_OF_TWO_SOFT_H

#include "ruleofbase.h"

namespace clang {
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
    explicit RuleOfTwoSoft(const std::string &name, const clang::CompilerInstance &ci);
    void VisitStmt(clang::Stmt *s) override;
};

#endif
