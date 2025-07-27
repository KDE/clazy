/*
    SPDX-FileCopyrightText: 2015 Klarälvdalens Datakonsult AB a KDAB Group company info@kdab.com
    SPDX-FileContributor: Sérgio Martins <sergio.martins@kdab.com>

    SPDX-FileCopyrightText: 2015 Sergio Martins <smartins@kde.org>
    SPDX-FileCopyrightText: 2025 Alexander Lohnau <alexander.lohnau@gmx.de>

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

std::vector<FixItHint> DetachingBase::getFixitHints(StringRef className, StringRef functionName, CallExpr *callExpr) const
{
    std::vector<FixItHint> hints;
    const auto &withCounterparts = clazy::detachingMethodsWithConstCounterParts();
    const auto &functionsIt = withCounterparts.find(className.str());

    if (functionsIt != withCounterparts.end()) {
        const auto functions = functionsIt->second;
        const auto it = clazy::find_if(functions, [functionName](const auto &pair) {
            return pair.first == functionName;
        });
        if (it == functions.end() || it->first == it->second) {
            // No fixit possible
        } else if (auto *memberCall = dyn_cast<CXXMemberCallExpr>(callExpr)) {
            if (auto *callee = dyn_cast<MemberExpr>(memberCall->getCallee())) {
                hints.emplace_back(FixItHint::CreateReplacement(callee->getMemberLoc(), it->second));
            }
        } else if (auto *operatorCall = dyn_cast<CXXOperatorCallExpr>(callExpr); operatorCall && it->first == "operator[]") {
            StringRef argText = Lexer::getSourceText(CharSourceRange::getTokenRange(operatorCall->getArg(1)->getSourceRange()), sm(), lo());
            SourceLocation left;
            for (SourceLocation beginLoc = operatorCall->getArg(0)->getBeginLoc(); beginLoc < operatorCall->getEndLoc();) {
                if (SourceLocation lLoc = Lexer::findLocationAfterToken(beginLoc, tok::l_square, sm(), lo(), false); lLoc.isValid()) {
                    left = lLoc.getLocWithOffset(-1);
                    break;
                } else {
                    beginLoc = Lexer::getLocForEndOfToken(beginLoc, /*Offset*/ 0, sm(), lo());
                }
            }

            SourceLocation right = operatorCall->getRParenLoc();
            std::string operatorRepleacement = ("." + it->second + "(" + argText + ")").str();
            hints.emplace_back(FixItHint::CreateReplacement(SourceRange(left, right), operatorRepleacement));
        }
    }
    return hints;
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
