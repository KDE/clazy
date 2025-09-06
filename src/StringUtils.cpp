/*
    SPDX-FileCopyrightText: 2015 Klarälvdalens Datakonsult AB a KDAB Group company info@kdab.com
    SPDX-FileContributor: Sérgio Martins <sergio.martins@kdab.com>

    SPDX-FileCopyrightText: 2015 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "StringUtils.h"

#include <string>
#include <vector>

namespace clang
{
class LangOptions;
} // namespace clang

using namespace clang;

std::string clazy::simpleArgTypeName(clang::FunctionDecl *func, unsigned int index, const clang::LangOptions &lo)
{
    if (!func || index >= func->getNumParams()) {
        return {};
    }

    ParmVarDecl *param = func->getParamDecl(index);
    return simpleTypeName(param, lo);
}

bool clazy::anyArgIsOfSimpleType(clang::FunctionDecl *func, const std::string &simpleType, const clang::LangOptions &lo)
{
    if (!func) {
        return false;
    }

    return std::ranges::any_of(Utils::functionParameters(func), [simpleType, lo](ParmVarDecl *p) {
        return simpleTypeName(p, lo) == simpleType;
    });
}

bool clazy::anyArgIsOfAnySimpleType(clang::FunctionDecl *func, const std::vector<std::string> &simpleTypes, const clang::LangOptions &lo)
{
    if (!func) {
        return false;
    }

    return std::ranges::any_of(simpleTypes, [func, lo](const std::string &simpleType) {
        return clazy::anyArgIsOfSimpleType(func, simpleType, lo);
    });
}
