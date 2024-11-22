/*
    SPDX-FileCopyrightText: 2015 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLANG_LAZY_AUTO_UNEXPECTED_QSTRING_BUILDER_H
#define CLANG_LAZY_AUTO_UNEXPECTED_QSTRING_BUILDER_H

#include "checkbase.h"

#include <string>

/**
 * Finds places where auto is deduced to be QStringBuilder instead of QString, which introduces crashes.
 *
 * See README-auto-unexpected-qstring-builder for more information
 */
class AutoUnexpectedQStringBuilder : public CheckBase
{
public:
    explicit AutoUnexpectedQStringBuilder(const std::string &name, ClazyContext *context);
    void VisitDecl(clang::Decl *decl) override;
    void VisitStmt(clang::Stmt *stmt) override;
};

#endif
