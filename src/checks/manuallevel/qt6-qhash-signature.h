/*
    This file is part of the clazy static checker.

    SPDX-FileCopyrightText: 2020 The Qt Company Ltd.
    SPDX-FileCopyrightText: 2020 Lucie Gerard <lucie.gerard@qt.io>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLAZY_QT6_QHASH_SIGNATURE
#define CLAZY_QT6_QHASH_SIGNATURE

#include "checkbase.h"

#include <string>
#include <vector>

class ClazyContext;

namespace clang
{
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
class Qt6QHashSignature : public CheckBase
{
public:
    explicit Qt6QHashSignature(const std::string &name, ClazyContext *context);
    void VisitDecl(clang::Decl *decl) override;
    void VisitStmt(clang::Stmt *stmt) override;

private:
    std::vector<clang::FixItHint> fixitReplace(clang::FunctionDecl *funcDecl, bool changeReturnType, bool changeParamType);
};

#endif
