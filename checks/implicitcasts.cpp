/**********************************************************************
**  Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
**  Author: Sérgio Martins <sergio.martins@kdab.com>
**
** This file may be distributed and/or modified under the terms of the
** GNU Lesser General Public License version 2.1 and version 3 as published by the
** Free Software Foundation and appearing in the file LICENSE.LGPL.txt included.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**********************************************************************/

#include "implicitcasts.h"
#include "Utils.h"
#include "checkmanager.h"
#include "StringUtils.h"

#include <clang/AST/AST.h>
#include <clang/Lex/Lexer.h>

using namespace clang;
using namespace std;


ImplicitCasts::ImplicitCasts(const std::string &name)
    : CheckBase(name)
{

}

void ImplicitCasts::VisitStmt(clang::Stmt *stmt)
{
    auto implicitCast = dyn_cast<ImplicitCastExpr>(stmt);
    if (implicitCast == nullptr)
        return;

    if (implicitCast->getCastKind() == clang::CK_LValueToRValue)
        return;

    if (implicitCast->getType().getTypePtrOrNull()->isBooleanType())
        return;

    Expr *expr = implicitCast->getSubExpr();
    QualType qt = expr->getType();

    if (!qt.getTypePtrOrNull()->isBooleanType()) // Filter out some bool to const bool
        return;

    Stmt *p = Utils::parent(m_parentMap, stmt);
    if (p && isa<BinaryOperator>(p))
        return;

    if (p && (isa<CStyleCastExpr>(p) || isa<CXXFunctionalCastExpr>(p)))
        return;

    if (Utils::isInsideOperatorCall(m_parentMap, stmt, {"QTextStream", "QAtomicInt", "QBasicAtomicInt"}))
        return;

    if (Utils::insideCTORCall(m_parentMap, stmt, {"QAtomicInt", "QBasicAtomicInt"}))
        return;

    if (Utils::parent(m_parentMap, implicitCast) == nullptr)
        return;

    StringUtils::printLocation(stmt->getLocStart());

    EnumConstantDecl *enumerator = m_lastDecl ? dyn_cast<EnumConstantDecl>(m_lastDecl) : nullptr;
    if (enumerator) {
        // False positive in Qt headers which generates a lot of noise
        return;
    }

    auto macro = Lexer::getImmediateMacroName(stmt->getLocStart(), m_ci.getSourceManager(), m_ci.getLangOpts());
    if (macro == "Q_UNLIKELY" || macro == "Q_LIKELY") {
        return;
    }

    emitWarning(stmt->getLocStart(), "Implicit cast from bool");
}

std::vector<string> ImplicitCasts::filesToIgnore() const
{
    static vector<string> files = {"/gcc/", "/c++/", "functional_hash.h", "qobject_impl.h", "qdebug.h",
                                   "hb-", "qdbusintegrator.cpp", "harfbuzz-", "qunicodetools.cpp"};
    return files;
}

REGISTER_CHECK("implicit-casts", ImplicitCasts)
