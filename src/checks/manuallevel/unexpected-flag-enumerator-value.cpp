/*
  This file is part of the clazy static checker.

  Copyright (C) 2021 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
  Author: Waqar Ahmed <waqar.ahmed@kdab.com>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.
*/

#include "unexpected-flag-enumerator-value.h"
#include "Utils.h"
#include "HierarchyUtils.h"
#include "QtUtils.h"
#include "TypeUtils.h"

#include <clang/AST/AST.h>

#include <algorithm>

using namespace clang;
using namespace std;


UnexpectedFlagEnumeratorValue::UnexpectedFlagEnumeratorValue(const std::string &name, ClazyContext *context)
    : CheckBase(name, context)
{
}

static ConstantExpr* getConstantExpr(EnumConstantDecl *enCD)
{
    auto cexpr = dyn_cast_or_null<ConstantExpr>(enCD->getInitExpr());
    if (cexpr)
        return cexpr;
    if (auto cast = dyn_cast_or_null<ImplicitCastExpr>(enCD->getInitExpr())) {
        return dyn_cast_or_null<ConstantExpr>(cast->getSubExpr());
    }
    return nullptr;
}

static bool isBinaryOperatorExpression(ConstantExpr* cexpr)
{
    auto subExpr = cexpr->getSubExpr();
    if (dyn_cast_or_null<BinaryOperator>(subExpr))
        return true;

    if (auto cast = dyn_cast_or_null<ImplicitCastExpr>(subExpr)) {
        return dyn_cast_or_null<BinaryOperator>(cast->getSubExpr());
    }
    return false;
}

static bool isReferenceToEnumerator(ConstantExpr* cexpr)
{
    return dyn_cast_or_null<DeclRefExpr>(cexpr->getSubExpr());
}

static bool isIntentionallyNotPowerOf2(EnumConstantDecl *en) {
    constexpr unsigned MinOnesToQualifyAsMask = 3;

    const auto val = en->getInitVal();
    if (val.isMask() && val.countTrailingOnes() >= MinOnesToQualifyAsMask)
        return true;

    if (val.isShiftedMask() && val.countPopulation() >= MinOnesToQualifyAsMask)
        return true;

    if (en->getName().contains_lower("mask"))
        return true;

    auto *cexpr = getConstantExpr(en);
    if (!cexpr)
        return false;

    if (isBinaryOperatorExpression(cexpr))
        return true;

    if (isReferenceToEnumerator(cexpr))
        return true;

    return false;
}

struct IsFlagEnumResult {
    bool isFlagEnum;
    int numFalseValues;
};

static SmallVector<EnumConstantDecl*, 16> getEnumerators(EnumDecl *enDecl)
{
    SmallVector<EnumConstantDecl*, 16> ret;
    for (auto *enumerator : enDecl->enumerators()) {
        ret.push_back(enumerator);
    }
    return ret;
}

static uint64_t getIntegerValue(EnumConstantDecl* e)
{
    return e->getInitVal().getLimitedValue();
}

static bool hasConsecutiveValues(const SmallVector<EnumConstantDecl*, 16>& enumerators)
{
    auto val = getIntegerValue(enumerators.front());
    const size_t until = std::min<size_t>(4, enumerators.size());
    for (size_t i = 1; i < until; ++i) {
        val++;
        if (getIntegerValue(enumerators[i]) != val)
            return false;
    }
    return true;
}

static IsFlagEnumResult isFlagEnum(const SmallVector<EnumConstantDecl*, 16>& enumerators)
{
    if (enumerators.size() < 4) {
        return {false, 0};
    }

    if (hasConsecutiveValues(enumerators)) {
        return {false, 0};
    }

    llvm::SmallVector<bool, 16> enumValues;
    for (auto *enumerator : enumerators) {
        enumValues.push_back(enumerator->getInitVal().isPowerOf2());
    }

    const size_t count = std::count(enumValues.begin(), enumValues.end(), false);

    // If half of our values were power-of-2, this is probably a flag enum
    IsFlagEnumResult res;
    res.isFlagEnum = count <= (enumerators.size() / 2);
    res.numFalseValues = count;
    return res;
}

void UnexpectedFlagEnumeratorValue::VisitDecl(clang::Decl *decl)
{
    auto enDecl = dyn_cast_or_null<EnumDecl>(decl);
    if (!enDecl || !enDecl->hasNameForLinkage())
        return;

    const SmallVector<EnumConstantDecl*, 16> enumerators = getEnumerators(enDecl);

    auto flagEnum = isFlagEnum(enumerators);
    if (!flagEnum.isFlagEnum)
        return;

    for (EnumConstantDecl* enumerator : enumerators) {
        const auto &initVal = enumerator->getInitVal();
        if (!initVal.isPowerOf2() && !initVal.isNullValue() && !initVal.isNegative()) {
            if (isIntentionallyNotPowerOf2(enumerator))
                continue;
            const auto value = enumerator->getInitVal().getLimitedValue();
            Expr *initExpr = enumerator->getInitExpr();
            emitWarning(initExpr ? initExpr->getBeginLoc() : enumerator->getBeginLoc(), "Unexpected non power-of-2 enumerator value: " + std::to_string(value));
        }
    }
}
