/*
  This file is part of the clazy static checker.

  Copyright (C) 2016 Sergio Martins <smartins@kde.org>

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

#include "incorrect-emit.h"

#if !defined(IS_OLD_CLANG)

#include "AccessSpecifierManager.h"
#include "Utils.h"
#include "HierarchyUtils.h"
#include "QtUtils.h"
#include "TypeUtils.h"
#include "checkmanager.h"

#include <clang/AST/AST.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/ExprCXX.h>
#include <clang/Lex/Lexer.h>

using namespace clang;
using namespace std;

IncorrectEmit::IncorrectEmit(const std::string &name, const clang::CompilerInstance &ci)
    : CheckBase(name, ci)
{
    m_checkManager->enableAccessSpecifierManager(ci);
    enablePreProcessorCallbacks();
    m_emitLocations.reserve(30); // bootstrap it
    m_filesToIgnore = { "moc_", ".moc" };
}

void IncorrectEmit::VisitMacroExpands(const Token &MacroNameTok, const SourceRange &range)
{
    IdentifierInfo *ii = MacroNameTok.getIdentifierInfo();
    if (ii && (ii->getName() == "emit" || ii->getName() == "Q_EMIT"))
        m_emitLocations.push_back(range.getBegin());
}

void IncorrectEmit::VisitStmt(Stmt *stmt)
{
    auto methodCall = dyn_cast<CXXMemberCallExpr>(stmt);
    if (!methodCall || !methodCall->getCalleeDecl())
        return;

    AccessSpecifierManager *accessSpecifierManager = m_checkManager->accessSpecifierManager();
    auto method = dyn_cast<CXXMethodDecl>(methodCall->getCalleeDecl());
    if (!method || !accessSpecifierManager)
        return;

    if (shouldIgnoreFile(stmt->getLocStart()))
        return;

    if (Stmt *parent = HierarchyUtils::parent(m_parentMap, methodCall)) {
        // Check if we're inside a chained call, such as: emit d_func()->mySignal()
        // We're not interested in the d_func() call, so skip it
        if (HierarchyUtils::getFirstParentOfType<CXXMemberCallExpr>(m_parentMap, parent))
            return;
    }

    const QtAccessSpecifierType type = accessSpecifierManager->qtAccessSpecifierType(method);
    if (type == QtAccessSpecifier_Unknown)
        return;

    const bool hasEmit = hasEmitKeyboard(methodCall);
    const string methodName = method->getQualifiedNameAsString();
    const bool isSignal = type == QtAccessSpecifier_Signal;
    if (isSignal && !hasEmit) {
        emitWarning(stmt, "Missing emit keyword on signal call " + methodName);
    } else if (!isSignal && hasEmit) {
        emitWarning(stmt, "Emit keyword being used with non-signal " + methodName);
    }

    if (isSignal)
        checkCallSignalInsideCTOR(methodCall);
}

void IncorrectEmit::checkCallSignalInsideCTOR(CXXMemberCallExpr *callExpr)
{
    if (!m_lastMethodDecl)
        return;

    auto ctorDecl = dyn_cast<CXXConstructorDecl>(m_lastMethodDecl);
    if (!ctorDecl)
        return;

    Expr *implicitArg = callExpr->getImplicitObjectArgument();
    if (!implicitArg || !isa<CXXThisExpr>(implicitArg)) // emit other->sig() is ok
        return;

    if (HierarchyUtils::getFirstParentOfType<LambdaExpr>(m_parentMap, callExpr) != nullptr)
        return; // Emit is inside a lambda, it's fine

    emitWarning(callExpr->getLocStart(), "Emitting inside constructor has no effect");
}

bool IncorrectEmit::hasEmitKeyboard(CXXMemberCallExpr *call) const
{
    SourceLocation callLoc = call->getLocStart();
    if (callLoc.isMacroID())
        callLoc = sm().getFileLoc(callLoc);

    for (const SourceLocation &emitLoc : m_emitLocations) {
        // We cache the calculation of the next token because it uses the Lexer and hence expensive.
        auto it = m_locationCache.find(emitLoc.getRawEncoding());

        SourceLocation nextTokenLoc;
        if (it == m_locationCache.end()) {
            nextTokenLoc = Utils::locForNextToken(emitLoc, sm(), lo());
            m_locationCache[emitLoc.getRawEncoding()] = nextTokenLoc;
        } else {
            nextTokenLoc = it->second;
        }

        if (nextTokenLoc == callLoc)
            return true;
    }

    return false;
}

REGISTER_CHECK_WITH_FLAGS("incorrect-emit", IncorrectEmit, CheckLevel1)

#endif
