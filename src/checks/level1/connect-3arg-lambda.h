/*
  This file is part of the clazy static checker.

    SPDX-FileCopyrightText: 2017 Sergio Martins <sergio.martins@kdab.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLAZY_CONNECT_3ARG_LAMBDA_H
#define CLAZY_CONNECT_3ARG_LAMBDA_H

#include "checkbase.h"

#include <string>

class ClazyContext;
namespace clang
{
class FunctionDecl;
class Stmt;
} // namespace clang

/**
 * See README-connect-3arg-lambda.md for more info.
 */
class Connect3ArgLambda : public CheckBase
{
public:
    explicit Connect3ArgLambda(const std::string &name, ClazyContext *context);
    void VisitStmt(clang::Stmt *stmt) override;

private:
    void processQTimer(clang::FunctionDecl *, clang::Stmt *);
    void processQMenu(clang::FunctionDecl *, clang::Stmt *);
};

#endif
