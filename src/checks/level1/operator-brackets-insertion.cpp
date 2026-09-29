/*
    SPDX-FileCopyrightText: 2026 Alexander Lohnau <alexander.lohnau@gmx.de>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "operator-brackets-insertion.h"
#include "HierarchyUtils.h"
#include "QtUtils.h"
#include "TypeUtils.h"
#include "Utils.h"

#include <clang/AST/AST.h>

using namespace clang;

namespace
{
bool isWriteLikeContext(Stmt *stmt, ParentMap *parentMap)
{
    auto *parent = clazy::parent(parentMap, stmt);
    if (!parent) {
        return false;
    }

    if (const auto *binary = dyn_cast<BinaryOperator>(parent)) {
        return binary->getOpcode() == BO_Assign;
    }

    if (dyn_cast<CompoundAssignOperator>(parent)) {
        return true;
    }

    if (const auto *unary = dyn_cast<UnaryOperator>(parent)) {
        return unary->isIncrementDecrementOp();
    }

    if (const auto *op = dyn_cast<CXXOperatorCallExpr>(parent)) {
        switch (op->getOperator()) {
        case OO_Equal:
        case OO_PlusEqual:
        case OO_MinusEqual:
        case OO_StarEqual:
        case OO_SlashEqual:
        case OO_PercentEqual:
        case OO_AmpEqual:
        case OO_PipeEqual:
        case OO_CaretEqual:
        case OO_LessLessEqual:
        case OO_GreaterGreaterEqual:
            return true;
        default:
            return false;
        }
    }

    return false;
}
}

bool OperatorBracketsInsertion::isAssociativeContainerOperatorSubscript(const CXXOperatorCallExpr *op) const
{
    const auto *callee = dyn_cast<CXXMethodDecl>(op->getDirectCallee());
    if (!callee) {
        return false;
    }

    const auto *record = callee->getParent();
    if (!record) {
        return false;
    }

    const StringRef className = clazy::name(record);
    return className == "QMap" || className == "QHash";
}

void OperatorBracketsInsertion::VisitStmt(clang::Stmt *stmt)
{
    if (!m_context || !m_context->parentMap) {
        return;
    }

    auto *op = dyn_cast<CXXOperatorCallExpr>(stmt);
    if (!op || op->getOperator() != OO_Subscript) {
        return;
    }

    if (!isAssociativeContainerOperatorSubscript(op)) {
        return;
    }
    if (isWriteLikeContext(op, m_context->parentMap)) {
        return;
    }

    emitWarning(op, "Using operator[] on a QMap/QHash in a read context inserts a value if the key is missing; use value() or contains() instead");
}
