/*
   This file is part of the clazy static checker.

  Copyright (C) 2016 Sergio Martins <smartins@kde.org>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.
*/

#ifndef CLAZY_UNUSED_NON_TRIVIAL_VARIABLE_H
#define CLAZY_UNUSED_NON_TRIVIAL_VARIABLE_H

#include "checkbase.h"

namespace clang {
class Stmt;
class VarDecl;
}

/**
 * Warns about unused Qt value classes.
 * Compilers usually only warn when trivial classes are unused and don't emit warnings for non-trivial classes.
 * This check has a whitelist of common Qt classes such as containers, QFont, QUrl, etc and warns for those too.
 *
 * See README-unused-non-trivial-variable for more information
 */
class UnusedNonTrivialVariable : public CheckBase
{
public:
    explicit UnusedNonTrivialVariable(const std::string &name, const clang::CompilerInstance &ci);
    void VisitStmt(clang::Stmt *stmt) override;
private:
    void handleVarDecl(clang::VarDecl *varDecl);
    bool isInterestingType(clang::QualType t) const;
};

#endif
