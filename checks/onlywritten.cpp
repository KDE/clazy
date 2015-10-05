/*
   This file is part of the clang-lazy static checker.

  Copyright (C) 2015 Sergio Martins <smartins@kde.org>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

  As a special exception, permission is given to link this program
  with any edition of Qt, and distribute the resulting executable,
  without including the source code for Qt in the source distribution.
*/

#include "onlywritten.h"
#include "Utils.h"
#include "checkmanager.h"

#include <clang/AST/DeclCXX.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/StmtCXX.h>
#include <clang/AST/ParentMap.h>
#include <clang/Lex/Lexer.h>
#include <string>
#include <vector>

using namespace clang;
using namespace std;

static bool isCandidateType(const QualType &qt)
{
    const Type *t = qt.getTypePtrOrNull();
    if (!t)
        return false;

    if (t->isBuiltinType() || t->isIntegralOrEnumerationType()) {
        return true;
    } else if (t->isRecordType()) {
        CXXRecordDecl *record = t->getAsCXXRecordDecl();
        if (record) {
            static const vector<string> permited = { "QList", "QVector", "QMap", "QHash", "QString",
                                                     "QByteArray", "QUrl", "QVarLengthArray", "QLinkedList",
                                                   "QPoint", "QRect", "QPointF", "QRectF"};
            return std::find(permited.cbegin(), permited.cend(), record->getNameAsString()) != permited.cend();
        }
    }

    return false;
}

OnlyWritten::OnlyWritten(const std::string &name)
    : CheckBase(name)
{
}

void OnlyWritten::VisitStmt(clang::Stmt *stm)
{
    DeclRefExpr *declRef = dyn_cast<DeclRefExpr>(stm);
    if (!declRef)
        return;

    ValueDecl *decl = declRef->getDecl();
    if (!decl || !isCandidateType(decl->getType()) || alreadyProcessed(decl))
        return;

    if (!Utils::isValueDeclInFunctionContext(decl))
        return;

    // Filter out stuff in ctor initializer lists.
    // CXXCtorInitializer for some reason isn't a Decl or a Stmt, so look for compound stmt instead
    auto compountStmt = Utils::getFirstParentOfType<CompoundStmt>(m_parentMap, declRef);
    if (!compountStmt)
        return;

    m_processedVariables.push_back(decl);
    FunctionDecl *func = dyn_cast<FunctionDecl>(decl->getDeclContext());
    Stmt *funcBody = func ? func->getBody() : nullptr;

    if (!hasStatementThatReadsVariable(nullptr, funcBody, decl, new ParentMap(funcBody))) {
        string msg = "Variable only written, not read (" + decl->getNameAsString() + ")";
        funcBody->dump();
        emitWarning(decl->getLocStart(), msg);
    }
}

bool OnlyWritten::hasStatementThatReadsVariable(Stmt *parentStmt, Stmt *stm, ValueDecl *variable, ParentMap *parentMap)
{
    if (!stm)
        return false;

    if (stm->getLocStart().isMacroID()) {
        auto macro = Lexer::getImmediateMacroName(stm->getLocStart(), m_ci.getSourceManager(), m_ci.getLangOpts());
        if (macro == "Q_UNUSED") // Don't warn on Q_UNUSED
            return true;
    }

    DeclRefExpr *declRef = dyn_cast<DeclRefExpr>(stm); 

    if (declRef && declRef->getDecl() == variable) {
        auto parent = Utils::parent(parentMap, stm);
        if (!parent) {
            // Clang bug ? Maybe, but we have access to the parent, so lets use it.
            parent = parentStmt;
        }

        if (parent) {
            auto implicitCast = dyn_cast<ImplicitCastExpr>(parent);
            if (implicitCast && implicitCast->getCastKind() == CK_LValueToRValue)
                return true;

            implicitCast = Utils::getFirstParentOfType<ImplicitCastExpr>(parentMap, declRef);
            if (implicitCast && implicitCast->getCastKind() == CK_LValueToRValue)
                return true;

            if (isa<ParenListExpr>(parent) || isa<CompoundAssignOperator>(parent))
                return true;

            auto unaryOperator = dyn_cast<UnaryOperator>(parent);
            if (unaryOperator && (unaryOperator->getOpcode() == UO_AddrOf || unaryOperator->getOpcode() == UO_Deref)) {
                return true;
            }

            auto cStyleCast = dyn_cast<CStyleCastExpr>(parent);
            if (cStyleCast && cStyleCast->getCastKind() == CK_ToVoid) {
                return true;
            }

            auto binaryOperator = dyn_cast<BinaryOperator>(parent);
            if (binaryOperator) {
                if (!binaryOperator->isAssignmentOp()) {
                    return true;
                }
            }

            auto constructExpr = Utils::getFirstParentOfType<CXXConstructExpr>(parentMap, declRef);
            if (constructExpr)
                return true;

            auto memberCall = Utils::getFirstParentOfType<CXXMemberCallExpr>(parentMap, declRef);
            if (memberCall) {
                if (memberCall->getImplicitObjectArgument() == declRef) {
                    // There's a member call to a variable we're analysing. Lets see if it's write only.
                    CXXMethodDecl *method = memberCall->getMethodDecl();
                    if (!method->getReturnType().getTypePtrOrNull()->isVoidType()) // Lets consider calling a non-void method as a read
                        return true;
                } else {
                    return true;
                }
            } else {
                // Catch any other callexpr
                if (Utils::getFirstParentOfType<CallExpr>(parentMap, declRef))
                    return true;
            }

            auto arraySubscript = Utils::getFirstParentOfType<ArraySubscriptExpr>(parentMap, declRef);
            if (arraySubscript)
                return true;

            if (Utils::getFirstParentOfType<CXXForRangeStmt>(parentMap, declRef))
                return true;

            if (Utils::getFirstParentOfType<IfStmt>(parentMap, declRef))
                return true;


            if (Utils::getFirstParentOfType<SwitchStmt>(parentMap, declRef))
                return true;

            if (Utils::getFirstParentOfType<DoStmt>(parentMap, declRef))
                return true;

            if (Utils::getFirstParentOfType<WhileStmt>(parentMap, declRef))
                return true;

            if (Utils::getFirstParentOfType<ReturnStmt>(parentMap, declRef))
                return true;

            if (Utils::getFirstParentOfType<ReturnStmt>(parentMap, declRef))
                return true;

            if (Utils::getFirstParentOfType<ConditionalOperator>(parentMap, declRef))
                return true;


            if (Utils::getFirstParentOfType<InitListExpr>(parentMap, declRef))
                return true;

            ForStmt *forStm = Utils::getFirstParentOfType<ForStmt>(parentMap, declRef);
            if (forStm) {
                if (Utils::isChildOf(declRef, forStm->getCond()) ||
                    Utils::isChildOf(declRef, forStm->getInc())  ||
                    Utils::isChildOf(declRef, forStm->getInit())) {
                    return true;
                }
            }

            // Check if it's in the RHS of an assignment
            binaryOperator = Utils::getFirstParentOfType<BinaryOperator>(parentMap, declRef);
            if (binaryOperator) {
                if (binaryOperator->getOpcode() == BO_Assign) {
                    if (binaryOperator->getRHS() == declRef || Utils::isChildOf(declRef, binaryOperator->getRHS()))
                        return true;
                } else {
                    return true;
                }
            }

        }
    }

    for (auto it = stm->child_begin(), end = stm->child_end(); it != end; ++it) {
        if (hasStatementThatReadsVariable(stm, *it, variable, parentMap))
            return true;
    }

    return false;
}

bool OnlyWritten::alreadyProcessed(ValueDecl *decl) const
{
    return std::find(m_processedVariables.cbegin(), m_processedVariables.cend(), decl) != m_processedVariables.cend();
}

REGISTER_CHECK_WITH_FLAGS("only-written", OnlyWritten, HiddenFlag)
