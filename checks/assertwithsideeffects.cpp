/*
   This file is part of the clang-lazy static checker.

  Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
  Author: Sérgio Martins <sergio.martins@kdab.com>

  Copyright (C) 2015 Sergio Martins <smartins@kde.org>

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

#include "assertwithsideeffects.h"
#include "Utils.h"
#include "StringUtils.h"
#include "checkmanager.h"

#include <clang/AST/Expr.h>
#include <clang/AST/Stmt.h>

using namespace clang;
using namespace std;


enum Aggressiveness
{
    NormalAggressiveness = 0,
    AlsoCheckFunctionCallsAggressiveness = 1 // too many false positives
};

AssertWithSideEffects::AssertWithSideEffects(const std::string &name, const clang::CompilerInstance &ci)
    : CheckBase(name, ci)
    , m_aggressiveness(NormalAggressiveness)
{
}

static bool functionIsOk(const string &name)
{
    static const vector<string> whitelist = {"qFuzzyIsNull", "qt_noop", "qt_assert", "qIsFinite", "qIsInf",
                                             "qIsNaN", "qIsNumericType", "operator==", "operator<", "operator>", "operator<=", "operator>=", "operator!=", "operator+", "operator-"
                                             "q_func", "d_func", "isEmptyHelper"
                                             "qCross", "qMin", "qMax", "qBound", "priv", "qobject_cast", "dbusService"};
    return find(whitelist.cbegin(), whitelist.cend(), name) != whitelist.cend();
}

static bool methodIsOK(const string &name)
{
    static const vector<string> whitelist = {"QList::begin", "QList::end", "QVector::begin",
                                             "QVector::end", "QHash::begin", "QHash::end",
                                             "QByteArray::data", "QBasicMutex::isRecursive",
                                             "QLinkedList::begin", "QLinkedList::end", "QDataBuffer::first",
                                            "QOpenGLFunctions::glIsRenderbuffer"};
    return find(whitelist.cbegin(), whitelist.cend(), name) != whitelist.cend();
}

void AssertWithSideEffects::VisitStmt(Stmt *stm)
{
    const SourceLocation stmStart = stm->getLocStart();
    if (!Utils::isInMacro(m_ci, stmStart, "Q_ASSERT"))
        return;

    bool warn = false;
    const bool checkfunctions = m_aggressiveness & AlsoCheckFunctionCallsAggressiveness;

    CXXMemberCallExpr *memberCall = dyn_cast<CXXMemberCallExpr>(stm);
    if (memberCall) {
        if (checkfunctions) {
            CXXMethodDecl *method = memberCall->getMethodDecl();
            if (!method->isConst() && !methodIsOK(StringUtils::qualifiedMethodName(method)) && !functionIsOk(method->getNameAsString())) {
                // llvm::errs() << "reason1 " << StringUtils::qualifiedMethodName(method) << "\n";
                warn = true;
            }
        }
    } else if (CallExpr *call = dyn_cast<CallExpr>(stm)) {
        // Non member function calls not allowed

        FunctionDecl *func = call->getDirectCallee();
        if (func && checkfunctions) {

            if (isa<CXXMethodDecl>(func)) // This will be visited next, so ignore it now
                return;

            const std::string funcName = func->getNameAsString();
            if (functionIsOk(funcName)) {
                return;
            }

            // llvm::errs() << "reason2 " << funcName << "\n";
            warn = true;
        }
    } else if (BinaryOperator *op = dyn_cast<BinaryOperator>(stm)) {
        if (op->isAssignmentOp()) {
            if (DeclRefExpr *declRef = dyn_cast<DeclRefExpr>(op->getLHS())) {
                ValueDecl *valueDecl = declRef->getDecl();
                if (valueDecl && m_ci.getSourceManager().isBeforeInSLocAddrSpace(valueDecl->getLocStart(), stmStart)) {
                    // llvm::errs() << "reason3\n";
                    warn = true;
                }
            }
        }
    } else if (UnaryOperator *op = dyn_cast<UnaryOperator>(stm)) {
        if (DeclRefExpr *declRef = dyn_cast<DeclRefExpr>(op->getSubExpr())) {
            ValueDecl *valueDecl = declRef->getDecl();
            auto type = op->getOpcode();
            if (type != UnaryOperatorKind::UO_Deref && type != UnaryOperatorKind::UO_AddrOf) {
                if (valueDecl && m_ci.getSourceManager().isBeforeInSLocAddrSpace(valueDecl->getLocStart(), stmStart)) {
                    // llvm::errs() << "reason5 " << op->getOpcodeStr() << "\n";
                    warn = true;
                }
            }
        }
    }

    if (warn) {
        emitWarning(stmStart, "Code inside Q_ASSERT has side-effects but won't be built in release mode");
    }
}

REGISTER_CHECK_WITH_FLAGS("assert-with-side-effects", AssertWithSideEffects, CheckLevel3)
