/*
    SPDX-FileCopyrightText: 2015 Albert Astals Cid <albert.astals@canonical.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "qdeleteall.h"
#include "ClazyContext.h"
#include "HierarchyUtils.h"
#include "QtUtils.h"
#include "SourceCompatibilityHelpers.h"
#include "StringUtils.h"

#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/Stmt.h>
#include <clang/Basic/LLVM.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Casting.h>

using namespace clang;

QDeleteAll::QDeleteAll(const std::string &name, ClazyContext *context)
    : CheckBase(name, context, Option_CanIgnoreIncludes)
{
}

void QDeleteAll::VisitStmt(clang::Stmt *stmt)
{
    // Find a call to QMap/QSet/QHash::values/keys
    auto *offendingCall = dyn_cast<CXXMemberCallExpr>(stmt);
    FunctionDecl *func = offendingCall ? offendingCall->getDirectCallee() : nullptr;
    if (!func) {
        return;
    }

    const std::string funcName = func->getNameAsString();
    const bool isValues = funcName == "values";
    if (!isValues && funcName != "keys") {
        return;
    }
    std::string offendingClassName;
    // In case QMultiHash::values is used in Qt5, values is defined in the QHash baseclass, look up the original record to be sure we have a QMultiHash
    if (auto *cast = dyn_cast<ImplicitCastExpr>(offendingCall->getImplicitObjectArgument())) {
        if (auto *subExpr = dyn_cast<DeclRefExpr>(cast->getSubExpr())) {
            if (auto *ptr = subExpr->getType().getTypePtrOrNull(); ptr && ptr->isRecordType()) {
                offendingClassName = ptr->getAsRecordDecl()->getNameAsString();
            }
        }
    }
    // Check if we have whitelisted the classname
    if (offendingClassName.empty() || !clazy::isQtAssociativeContainer(offendingClassName)) {
        return;
    }

    // Once found see if the first parent call is qDeleteAll
    int i = 1;
    Stmt *p = clazy::parent(m_context->parentMap, stmt, i);
    while (p) {
        auto *pc = dyn_cast<CallExpr>(p);
        if (FunctionDecl *f = pc ? pc->getDirectCallee() : nullptr) {
            if (clazy::name(f) == "qDeleteAll") {
                std::string msg = "qDeleteAll() is being used on an unnecessary temporary container created by " + offendingClassName + "::" + funcName + "()";
                if (func->getNumParams() == 0) { // Ignore values method calls where lookup-parameter is given, like the deprecated QHash::values(Key)
                    if (isValues) {
                        msg += ", use qDeleteAll(mycontainer) instead";
                    } else {
                        msg += ", use qDeleteAll(mycontainer.keyBegin(), mycontainer.keyEnd()) instead";
                    }
                    emitWarning(clazy::getLocStart(p), msg);
                }
            }
            break;
        }
        ++i;
        p = clazy::parent(m_context->parentMap, stmt, i);
    }
}
