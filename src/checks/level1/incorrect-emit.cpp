/*
    SPDX-FileCopyrightText: 2016 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "incorrect-emit.h"
#include "AccessSpecifierManager.h"
#include "ClazyContext.h"
#include "HierarchyUtils.h"
#include "Utils.h"

#include <clang/AST/DeclCXX.h>
#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/Stmt.h>
#include <clang/Basic/IdentifierTable.h>
#include <clang/Basic/LLVM.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Lex/Token.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Casting.h>

#include <utility>

namespace clang
{
class MacroInfo;
} // namespace clang

using namespace clang;

IncorrectEmit::IncorrectEmit(const std::string &name, ClazyContext *context)
    : CheckBase(name, context, Option_CanIgnoreIncludes)
{
    context->enableAccessSpecifierManager();
    enablePreProcessorCallbacks();
    m_emitLocations.reserve(30); // bootstrap it
    m_filesToIgnore = {"moc_", ".moc"};
}

void IncorrectEmit::VisitMacroExpands(const Token &MacroNameTok, const SourceRange &range, const MacroInfo *)
{
    IdentifierInfo *ii = MacroNameTok.getIdentifierInfo();
    if (ii && (ii->getName() == "emit" || ii->getName() == "Q_EMIT")) {
        m_emitLocations.push_back(range.getBegin());
    }
}

void IncorrectEmit::VisitStmt(Stmt *stmt)
{
    auto *methodCall = dyn_cast<CXXMemberCallExpr>(stmt);
    if (!methodCall || !methodCall->getCalleeDecl()) {
        return;
    }

    const AccessSpecifierManager *accessSpecifierManager = m_context->accessSpecifierManager;
    auto *method = dyn_cast<CXXMethodDecl>(methodCall->getCalleeDecl());
    if (!method || !accessSpecifierManager) {
        return;
    }

    if (shouldIgnoreFile(stmt->getBeginLoc())) {
        return;
    }

    if (Stmt *parent = clazy::parent(m_context->parentMap, methodCall)) {
        // Check if we're inside a chained call, such as: emit d_func()->mySignal()
        // We're not interested in the d_func() call, so skip it
        if (clazy::getFirstParentOfType<CXXMemberCallExpr>(m_context->parentMap, parent)) {
            return;
        }
    }

    const QtAccessSpecifierType type = accessSpecifierManager->qtAccessSpecifierType(method);
    if (type == QtAccessSpecifier_Unknown) {
        return;
    }

    const bool hasEmit = hasEmitKeyboard(methodCall);
    const std::string methodName = method->getQualifiedNameAsString();
    const bool isSignal = type == QtAccessSpecifier_Signal;
    if (isSignal && !hasEmit) {
        emitWarning(stmt, "Missing emit keyword on signal call " + methodName);
    } else if (!isSignal && hasEmit) {
        emitWarning(stmt, "Emit keyword being used with non-signal " + methodName);
    }

    if (isSignal) {
        checkCallSignalInsideCTOR(methodCall);
    }
}

void IncorrectEmit::checkCallSignalInsideCTOR(CXXMemberCallExpr *callExpr)
{
    if (!m_context->lastMethodDecl) {
        return;
    }

    auto *ctorDecl = dyn_cast<CXXConstructorDecl>(m_context->lastMethodDecl);
    if (!ctorDecl) {
        return;
    }

    Expr *implicitArg = callExpr->getImplicitObjectArgument();
    if (!implicitArg || !isa<CXXThisExpr>(implicitArg)) { // emit other->sig() is ok
        return;
    }

    if (clazy::getFirstParentOfType<LambdaExpr>(m_context->parentMap, callExpr) != nullptr) {
        return; // Emit is inside a lambda, it's fine
    }

    emitWarning(callExpr->getBeginLoc(), "Emitting inside constructor probably has no effect");
}

bool IncorrectEmit::hasEmitKeyboard(CXXMemberCallExpr *call) const
{
    SourceLocation callLoc = call->getBeginLoc();
    if (callLoc.isMacroID()) {
        callLoc = sm().getFileLoc(callLoc);
    }

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

        if (nextTokenLoc == callLoc) {
            return true;
        }
    }

    return false;
}
