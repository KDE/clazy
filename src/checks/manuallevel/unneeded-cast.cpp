/*
    SPDX-FileCopyrightText: 2015 Klarälvdalens Datakonsult AB a KDAB Group company info@kdab.com
    SPDX-FileContributor: Sérgio Martins <sergio.martins@kdab.com>

    SPDX-FileCopyrightText: 2015 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "unneeded-cast.h"
#include "ClazyContext.h"
#include "HierarchyUtils.h"
#include "QtUtils.h"
#include "SourceCompatibilityHelpers.h"
#include "TypeUtils.h"
#include "Utils.h"

#include <clang/AST/DeclCXX.h>
#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/OperationKinds.h>
#include <clang/AST/Stmt.h>
#include <clang/Basic/SourceLocation.h>
#include <llvm/Support/Casting.h>

#include <iterator>

using namespace llvm;
using namespace clang;

// Returns if callExpr is a call to qobject_cast()
inline bool is_qobject_cast(clang::Stmt *s, clang::CXXRecordDecl **castTo = nullptr, clang::CXXRecordDecl **castFrom = nullptr)
{
    if (auto *callExpr = llvm::dyn_cast<clang::CallExpr>(s)) {
        clang::FunctionDecl *func = callExpr->getDirectCallee();
        if (!func || clazy::name(func) != "qobject_cast") {
            return false;
        }

        if (castFrom) {
            clang::Expr *expr = callExpr->getArg(0);
            if (auto *implicitCast = llvm::dyn_cast<clang::ImplicitCastExpr>(expr)) {
                if (implicitCast->getCastKind() == clang::CK_DerivedToBase) {
                    expr = implicitCast->getSubExpr();
                }
            }
            clang::QualType qt = clazy::pointeeQualType(expr->getType());
            if (!qt.isNull()) {
                clang::CXXRecordDecl *record = qt->getAsCXXRecordDecl();
                *castFrom = record ? record->getCanonicalDecl() : nullptr;
            }
        }

        if (castTo) {
            const auto *templateArgs = func->getTemplateSpecializationArgs();
            if (templateArgs->size() == 1) {
                const clang::TemplateArgument &arg = templateArgs->get(0);
                clang::QualType qt = clazy::pointeeQualType(arg.getAsType());
                if (!qt.isNull()) {
                    clang::CXXRecordDecl *record = qt->getAsCXXRecordDecl();
                    *castTo = record ? record->getCanonicalDecl() : nullptr;
                }
            }
        }
        return true;
    }

    return false;
}

UnneededCast::UnneededCast(const std::string &name, ClazyContext *context)
    : CheckBase(name, context, Option_CanIgnoreIncludes)
{
}

void UnneededCast::VisitStmt(clang::Stmt *stm)
{
    if (handleNamedCast(dyn_cast<CXXNamedCastExpr>(stm))) {
        return;
    }

    handleQObjectCast(stm);
}

bool UnneededCast::handleNamedCast(CXXNamedCastExpr *namedCast)
{
    if (!namedCast) {
        return false;
    }

    const bool isDynamicCast = isa<CXXDynamicCastExpr>(namedCast);
    const bool isStaticCast = isDynamicCast ? false : isa<CXXStaticCastExpr>(namedCast);

    if (!isDynamicCast && !isStaticCast) {
        return false;
    }

    if (clazy::getLocStart(namedCast).isMacroID()) {
        return false;
    }

    CXXRecordDecl *castFrom = namedCast ? Utils::namedCastInnerDecl(namedCast) : nullptr;
    if (!castFrom || !castFrom->hasDefinition() || std::distance(castFrom->bases_begin(), castFrom->bases_end()) > 1) {
        return false;
    }

    if (isStaticCast) {
        if (auto *implicitCast = dyn_cast<ImplicitCastExpr>(namedCast->getSubExpr())) {
            if (implicitCast->getCastKind() == CK_NullToPointer) {
                // static_cast<Foo*>(0) is OK, and sometimes needed
                return false;
            }
        }

        // static_cast to base is needed in ternary operators
        if (clazy::getFirstParentOfType<ConditionalOperator>(m_context->parentMap, namedCast) != nullptr) {
            return false;
        }
    }

    if (isDynamicCast && !isOptionSet("prefer-dynamic-cast-over-qobject") && clazy::isQObject(castFrom)) {
        emitWarning(clazy::getLocStart(namedCast), "Use qobject_cast rather than dynamic_cast");
    }

    CXXRecordDecl *castTo = Utils::namedCastOuterDecl(namedCast);
    if (!castTo) {
        return false;
    }

    return maybeWarn(namedCast, castFrom, castTo);
}

bool UnneededCast::handleQObjectCast(Stmt *stm)
{
    CXXRecordDecl *castTo = nullptr;
    CXXRecordDecl *castFrom = nullptr;

    if (!is_qobject_cast(stm, &castTo, &castFrom)) {
        return false;
    }

    return maybeWarn(stm, castFrom, castTo, /*isQObjectCast=*/true);
}

bool UnneededCast::maybeWarn(Stmt *stmt, CXXRecordDecl *castFrom, CXXRecordDecl *castTo, bool isQObjectCast)
{
    castFrom = castFrom->getCanonicalDecl();
    castTo = castTo->getCanonicalDecl();

    if (castFrom == castTo) {
        emitWarning(clazy::getLocStart(stmt), "Casting to itself");
        return true;
    }
    if (clazy::derivesFrom(/*child=*/castFrom, castTo)) {
        if (isQObjectCast) {
            const bool isTernaryOperator = clazy::getFirstParentOfType<ConditionalOperator>(m_context->parentMap, stmt) != nullptr;
            if (isTernaryOperator) {
                emitWarning(clazy::getLocStart(stmt), "use static_cast instead of qobject_cast");
            } else {
                emitWarning(clazy::getLocStart(stmt), "explicitly casting to base is unnecessary");
            }
        } else {
            emitWarning(clazy::getLocStart(stmt), "explicitly casting to base is unnecessary");
        }

        return true;
    }

    return false;
}
