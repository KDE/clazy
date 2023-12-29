/*
    SPDX-FileCopyrightText: 2018 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLAZY_LOWERCASE_QML_TYPE_NAME_H
#define CLAZY_LOWERCASE_QML_TYPE_NAME_H

#include "checkbase.h"

/**
 * See README-lowercase-qml-type-name.md for more info.
 */
class LowercaseQMlTypeName : public CheckBase
{
public:
    explicit LowercaseQMlTypeName(const std::string &name, ClazyContext *context);
    void VisitStmt(clang::Stmt *) override;

private:
};

#endif
