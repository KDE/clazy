/*
    SPDX-FileCopyrightText: 2015 Klarälvdalens Datakonsult AB a KDAB Group company info@kdab.com
    SPDX-FileContributor: Sérgio Martins <sergio.martins@kdab.com>

    SPDX-FileCopyrightText: 2015 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef FUNCTION_ARGS_BY_REF_H
#define FUNCTION_ARGS_BY_REF_H

#include "checkbase.h"

#include <string>

namespace clang
{
class VarDecl;
class FixItHint;
class ParmVarDecl;
class FunctionDecl;
}

namespace clazy
{
struct QualTypeClassification;
}

/**
 * Finds functions where big non-trivial types are passed by value instead of const-ref.
 * Looks into the body of the functions to see if the argument are read-only, it doesn't emit a warning otherwise.
 */
class FunctionArgsByRef : public CheckBase
{
public:
    FunctionArgsByRef(const std::string &name, ClazyContext *context);
    void VisitDecl(clang::Decl *decl) override;
    void VisitStmt(clang::Stmt *stmt) override;

private:
    static bool shouldIgnoreClass(clang::CXXRecordDecl *);
    static bool shouldIgnoreOperator(clang::FunctionDecl *);
    static bool shouldIgnoreFunction(clang::FunctionDecl *);
    void processFunction(clang::FunctionDecl *);
    void addFixits(std::vector<clang::FixItHint> &fixits, clang::FunctionDecl *, unsigned int parmIndex);
    clang::FixItHint fixit(const clang::ParmVarDecl *, clazy::QualTypeClassification);
};

#endif
