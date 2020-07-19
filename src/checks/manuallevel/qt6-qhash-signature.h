/*
    This file is part of the clazy static checker.

    Copyright (C) 2020 The Qt Company Ltd.
    Copyright (C) 2020 Lucie Gerard <lucie.gerard@qt.io>

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

#ifndef CLAZY_QT6_QHASH_SIGNATURE
#define CLAZY_QT6_QHASH_SIGNATURE

#include "checkbase.h"

#include <vector>
#include <string>

class ClazyContext;

namespace clang {
class Stmt;
class FixItHint;
class CXXConstructExpr;
class CXXOperatorCallExpr;
class Expr;
class CXXMemberCallExpr;

class CXXFunctionalCastExpr;
}

/**
 * Replaces qhash signature uint with size_t.
 *
 * Run only in Qt 6 code.
 */
class Qt6QHashSignature
    : public CheckBase
{
public:
    explicit Qt6QHashSignature(const std::string &name, ClazyContext *context);
    void VisitDecl(clang::Decl *decl) override;
    void VisitStmt(clang::Stmt *stmt) override;

private:
    std::vector<clang::FixItHint> fixitReplace(clang::FunctionDecl *funcDecl, bool changeReturnType, bool changeParamType);
};

#endif
