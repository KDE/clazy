/*
   This file is part of the clazy static checker.

  Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
  Author: Sérgio Martins <sergio.martins@kdab.com>

  Copyright (C) 2015 Sergio Martins <smartins@kde.org>

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

#ifndef FUNCTION_ARGS_BY_REF_H
#define FUNCTION_ARGS_BY_REF_H

#include "checkbase.h"

namespace clang {
class Decl;
class VarDecl;
class FixItHint;
class ParmVarDecl;
class FunctionDecl;
}

namespace Utils {
struct QualTypeClassification;
}

/**
 * Finds functions where big non-trivial types are passed by value instead of const-ref.
 * Looks into the body of the functions to see if the argument are read-only, it doesn't emit a warning otherwise.
 */
class FunctionArgsByRef : public CheckBase
{
public:
    FunctionArgsByRef(const std::string &name, const clang::CompilerInstance &ci);
    void VisitDecl(clang::Decl *decl) override;
    void VisitStmt(clang::Stmt *stmt) override;
protected:
    std::vector<std::string> filesToIgnore() const override;
private:
    void processFunction(clang::FunctionDecl *);
    clang::FixItHint fixitByValue(clang::FunctionDecl *func, const clang::ParmVarDecl *param, const Utils::QualTypeClassification &);
    clang::FixItHint fixitByConstRef(const clang::ParmVarDecl *, const Utils::QualTypeClassification &);
};

#endif
