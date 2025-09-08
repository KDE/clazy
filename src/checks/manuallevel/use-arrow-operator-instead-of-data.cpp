/*
    SPDX-FileCopyrightText: 2021 Klarälvdalens Datakonsult AB a KDAB Group company info@kdab.com
    SPDX-FileContributor: Waqar Ahmed <waqar.ahmed@kdab.com>

    SPDX-FileCopyrightText: 2021 Waqar Ahmed <waqar.17a@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "use-arrow-operator-instead-of-data.h"
#include "StringUtils.h"
#include "Utils.h"
#include "clazy_stl.h"

#include <clang/AST/ExprCXX.h>

using namespace clang;

void UseArrowOperatorInsteadOfData::VisitStmt(clang::Stmt *stmt)
{
    auto *ce = dyn_cast<CXXMemberCallExpr>(stmt);
    if (!ce) {
        return;
    }

    auto vec = Utils::callListForChain(ce);
    if (vec.size() < 2) {
        return;
    }

    CallExpr *callExpr = vec.at(vec.size() - 1);

    FunctionDecl *funcDecl = callExpr->getDirectCallee();
    if (!funcDecl) {
        return;
    }
    const std::string func = clazy::qualifiedMethodName(funcDecl);
    static const std::vector<std::string> whiteList{"QScopedPointer::data", "QPointer::data", "QSharedPointer::data", "QSharedDataPointer::data"};
    if (!clazy::contains(whiteList, func)) {
        return;
    }

    for (const auto &chain : vec) {
        if (auto a = dyn_cast<CXXMemberCallExpr>(chain)) {
            if (auto callee = dyn_cast<MemberExpr>(a->getCallee()); callee && dyn_cast<CXXStaticCastExpr>(callee->getBase()->IgnoreImpCasts())) {
                return; // We have some kind of cast going on here. This means we can not use the arrow operator directly
            }
        }
    }

    std::vector<FixItHint> fixits;

    constexpr int MinPossibleColonPos = sizeof("QPointer") - 1;
    const std::string ClassName = func.substr(0, func.find(':', MinPossibleColonPos));

    auto begin = callExpr->getExprLoc();
    const auto end = callExpr->getEndLoc();

    // find '.' in ptr.data()
    int dotOffset = 0;
    const char *d = sm().getCharacterData(begin);
    while (*d != '.') {
        dotOffset--;
        d--;
    }
    begin = begin.getLocWithOffset(dotOffset);

    fixits.push_back(FixItHint::CreateRemoval(SourceRange{begin, end}));

    emitWarning(callExpr->getBeginLoc(), "Use operator -> directly instead of " + ClassName + "::data()->", fixits);
}
