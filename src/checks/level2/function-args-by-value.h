/*
    SPDX-FileCopyrightText: 2016 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLAZY_FUNC_ARGS_BY_VALUE_H
#define CLAZY_FUNC_ARGS_BY_VALUE_H

#include "checkbase.h"

#include <clang/Basic/Diagnostic.h>

#include <string>

namespace clang
{
class FunctionDecl;
class ParmVarDecl;
}

namespace clazy
{
struct QualTypeClassification;
}

/**
 * Finds arguments that should be passed by value instead of const-ref
 *
 * See README-function-args-by-value for more info
 */
class FunctionArgsByValue : public CheckBase
{
public:
    explicit FunctionArgsByValue(const std::string &name, ClazyContext *context);
    void VisitDecl(clang::Decl *decl) override;
    void VisitStmt(clang::Stmt *stmt) override;

private:
    void processFunction(clang::FunctionDecl *);
    static bool shouldIgnoreClass(clang::CXXRecordDecl *);
    static bool shouldIgnoreOperator(clang::FunctionDecl *);
    static bool shouldIgnoreFunction(clang::FunctionDecl *);
    clang::FixItHint fixit(clang::FunctionDecl *func, const clang::ParmVarDecl *param, clazy::QualTypeClassification);
};

#endif
