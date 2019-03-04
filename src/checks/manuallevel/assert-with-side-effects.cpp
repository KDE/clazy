/*
    This file is part of the clazy static checker.

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

#include "assert-with-side-effects.h"
#include "MacroUtils.h"
#include "StringUtils.h"
#include "SourceCompatibilityHelpers.h"
#include "clazy_stl.h"

#include <clang/AST/Expr.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/OperationKinds.h>
#include <clang/Basic/LLVM.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/SourceManager.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Casting.h>

#include <vector>

class ClazyContext;

using namespace clang;
using namespace std;


enum Aggressiveness
{
    NormalAggressiveness = 0,
    AlsoCheckFunctionCallsAggressiveness = 1 // too many false positives
};

AssertWithSideEffects::AssertWithSideEffects(const std::string &name, ClazyContext *context)
    : CheckBase(name, context, Option_CanIgnoreIncludes)
    , m_aggressiveness(NormalAggressiveness)
{
}

static bool functionIsOk(StringRef name)
{
    static const vector<StringRef> whitelist = {"qFuzzyIsNull", "qt_noop", "qt_assert", "qIsFinite", "qIsInf",
                                                "qIsNaN", "qIsNumericType", "operator==", "operator<", "operator>", "operator<=", "operator>=", "operator!=", "operator+", "operator-"
                                                "q_func", "d_func", "isEmptyHelper"
                                                "qCross", "qMin", "qMax", "qBound", "priv", "qobject_cast", "dbusService"};
    return clazy::contains(whitelist, name);
}

static bool methodIsOK(const string &name)
{
    static const vector<string> whitelist = {"QList::begin", "QList::end", "QVector::begin",
                                             "QVector::end", "QHash::begin", "QHash::end",
                                             "QByteArray::data", "QBasicMutex::isRecursive",
                                             "QLinkedList::begin", "QLinkedList::end", "QDataBuffer::first",
                                             "QOpenGLFunctions::glIsRenderbuffer"};
    return clazy::contains(whitelist, name);
}

void AssertWithSideEffects::VisitStmt(Stmt *stm)
{
    const SourceLocation stmStart = clazy::getLocStart(stm);
    if (!clazy::isInMacro(&m_astContext, stmStart, "Q_ASSERT"))
        return;

    bool warn = false;
    const bool checkfunctions = m_aggressiveness & AlsoCheckFunctionCallsAggressiveness;

    auto memberCall = dyn_cast<CXXMemberCallExpr>(stm);
    if (memberCall) {
        if (checkfunctions) {
            CXXMethodDecl *method = memberCall->getMethodDecl();
            if (!method->isConst() && !methodIsOK(clazy::qualifiedMethodName(method)) && !functionIsOk(clazy::name(method))) {
                // llvm::errs() << "reason1 " << clazy::qualifiedMethodName(method) << "\n";
                warn = true;
            }
        }
    } else if (auto call = dyn_cast<CallExpr>(stm)) {
        // Non member function calls not allowed

        FunctionDecl *func = call->getDirectCallee();
        if (func && checkfunctions) {

            if (isa<CXXMethodDecl>(func)) // This will be visited next, so ignore it now
                return;

            if (functionIsOk(clazy::name(func))) {
                return;
            }

            warn = true;
        }
    } else if (auto op = dyn_cast<BinaryOperator>(stm)) {
        if (op->isAssignmentOp()) {
            if (DeclRefExpr *declRef = dyn_cast<DeclRefExpr>(op->getLHS())) {
                ValueDecl *valueDecl = declRef->getDecl();
                if (valueDecl && sm().isBeforeInSLocAddrSpace(clazy::getLocStart(valueDecl), stmStart)) {
                    // llvm::errs() << "reason3\n";
                    warn = true;
                }
            }
        }
    } else if (auto op = dyn_cast<UnaryOperator>(stm)) {
        if (auto declRef = dyn_cast<DeclRefExpr>(op->getSubExpr())) {
            ValueDecl *valueDecl = declRef->getDecl();
            auto type = op->getOpcode();
            if (type != UnaryOperatorKind::UO_Deref && type != UnaryOperatorKind::UO_AddrOf) {
                if (valueDecl && sm().isBeforeInSLocAddrSpace(clazy::getLocStart(valueDecl), stmStart)) {
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
