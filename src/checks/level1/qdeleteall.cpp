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
    const bool isKeys = isValues ? false : funcName == "keys";

    if (isValues || isKeys) {
        const std::string offendingClassName = offendingCall->getMethodDecl()->getParent()->getNameAsString();
        if (clazy::isQtAssociativeContainer(offendingClassName)) {
            // Once found see if the first parent call is qDeleteAll
            int i = 1;
            Stmt *p = clazy::parent(m_context->parentMap, stmt, i);
            while (p) {
                auto *pc = dyn_cast<CallExpr>(p);
                FunctionDecl *f = pc ? pc->getDirectCallee() : nullptr;
                if (f) {
                    if (clazy::name(f) == "qDeleteAll") {
                        std::string msg =
                            "qDeleteAll() is being used on an unnecessary temporary container created by " + offendingClassName + "::" + funcName + "()";
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
    }
}
