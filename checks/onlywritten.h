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

#ifndef CLANG_LAZY_ONLY_WRITTEN_H
#define CLANG_LAZY_ONLY_WRITTEN_H

#include "checkbase.h"
#include <vector>

namespace clang {
class ValueDecl;
}

/**
 * Finds places where variables are only written, not read.
 */
class OnlyWritten : public CheckBase
{
public:
    OnlyWritten(const std::string &name);
    void VisitStmt(clang::Stmt *stm) override;
private:
    bool hasStatementThatReadsVariable(clang::Stmt *parentStmt, clang::Stmt *stm, clang::ValueDecl *variable, clang::ParentMap *parentMap);
    bool alreadyProcessed(clang::ValueDecl *decl) const;
    std::vector<clang::ValueDecl*> m_processedVariables;
};

#endif
