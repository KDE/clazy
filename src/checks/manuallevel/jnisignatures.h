/*
    SPDX-FileCopyrightText: 2020 Klarälvdalens Datakonsult AB a KDAB Group company info@kdab.com
    SPDX-FileContributor: Nicolas Fella <nicolas.fella@kdab.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef JNISIGNATURES_H
#define JNISIGNATURES_H

#include "checkbase.h"

#include <regex>
#include <string>

class JniSignatures : public CheckBase
{
public:
    using CheckBase::CheckBase;
    void VisitStmt(clang::Stmt *) override;

private:
    template<typename T>
    void checkArgAt(T *call, unsigned int index, const std::regex &expr, const std::string &errorMessage);
    void checkConstructorCall(clang::Stmt *stm);
    void checkFunctionCall(clang::Stmt *stm);
};

#endif
