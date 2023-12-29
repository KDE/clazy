/*
    SPDX-FileCopyrightText: 2015 Klarälvdalens Datakonsult AB a KDAB Group company info@kdab.com
    Author: Sérgio Martins <sergio.martins@kdab.com>

    SPDX-FileCopyrightText: 2015 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "detachingbase.h"
#include "QtUtils.h"
#include "StringUtils.h"
#include "clazy_stl.h"

#include <clang/AST/DeclCXX.h>
#include <llvm/ADT/StringRef.h>

#include <unordered_map>
#include <utility>
#include <vector>

class ClazyContext;

using namespace clang;

DetachingBase::DetachingBase(const std::string &name, ClazyContext *context, Options options)
    : CheckBase(name, context, options)
{
}

bool DetachingBase::isDetachingMethod(CXXMethodDecl *method, DetachingMethodType detachingMethodType) const
{
    if (!method) {
        return false;
    }

    CXXRecordDecl *record = method->getParent();
    if (!record) {
        return false;
    }

    StringRef className = clazy::name(record);

    const std::unordered_map<std::string, std::vector<StringRef>> &methodsByType =
        detachingMethodType == DetachingMethod ? clazy::detachingMethods() : clazy::detachingMethodsWithConstCounterParts();
    auto it = methodsByType.find(static_cast<std::string>(className));
    if (it != methodsByType.cend()) {
        const auto &methods = it->second;
        if (clazy::contains(methods, clazy::name(method))) {
            return true;
        }
    }

    return false;
}
