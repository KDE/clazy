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

#ifndef CLAZY_FUNC_ARGS_BY_VALUE_H
#define CLAZY_FUNC_ARGS_BY_VALUE_H

#include "checkbase.h"

#include <clang/Basic/Diagnostic.h>

#include <string>

class ClazyContext;

namespace clang {
class Stmt;
class Decl;
class FunctionDecl;
class ParmVarDecl;
}

namespace clazy {
struct QualTypeClassification;
}

/**
 * Finds arguments that should be passed by value instead of const-ref
 *
 * See README-function-args-by-value for more info
 */
class FunctionArgsByValue
    : public CheckBase
{
public:
    explicit FunctionArgsByValue(const std::string &name, ClazyContext *context);
    void VisitDecl(clang::Decl *decl) override;
    void VisitStmt(clang::Stmt *stmt) override;
private:
    void processFunction(clang::FunctionDecl *);
    clang::FixItHint fixit(clang::FunctionDecl *func, const clang::ParmVarDecl *param,
                           clazy::QualTypeClassification);
};

#endif
