/*
    SPDX-FileCopyrightText: 2015 Klarälvdalens Datakonsult AB a KDAB Group company info@kdab.com
    SPDX-FileContributor: Sérgio Martins <sergio.martins@kdab.com>

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

using namespace clang;

DetachingBase::DetachingBase(const std::string &name, Options options)
    : CheckBase(name, options)
{
}

bool DetachingBase::isDetachingMethod(CXXMethodDecl *method, DetachingMethodType detachingMethodType) const
{
    if (!method) {
        return false;
    }

    const CXXRecordDecl *record = method->getParent();
    if (!record) {
        return false;
    }

    StringRef className = clazy::name(record);
    StringRef methodName = clazy::name(method);

    if (detachingMethodType == DetachingMethod) {
        const auto &methodsByType = clazy::detachingMethods();
        auto it = methodsByType.find(className.str());
        if (it != methodsByType.cend()) {
            const auto &methods = it->second;
            return clazy::contains(methods, methodName);
        }
    }

    const auto &methodsByType = clazy::detachingMethodsWithConstCounterParts();

    auto it = methodsByType.find(className.str());
    if (it != methodsByType.cend()) {
        const auto &methods = it->second;
        return clazy::any_of(methods, [&methodName](auto pair) {
            return pair.first == methodName;
        });
    }

    return false;
}
