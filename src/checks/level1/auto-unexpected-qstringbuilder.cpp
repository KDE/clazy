/*
    SPDX-FileCopyrightText: 2015 Sergio Martins <smartins@kde.org>
    SPDX-FileCopyrightText: 2015 Mathias Hasselmann <mathias.hasselmann@kdab.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "auto-unexpected-qstringbuilder.h"
#include "FixItUtils.h"
#include "SourceCompatibilityHelpers.h"
#include "StringUtils.h"
#include "TypeUtils.h"

#include <clang/AST/Decl.h>
#include <clang/AST/DeclBase.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/Type.h>
#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/LLVM.h>
#include <clang/Basic/SourceLocation.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Casting.h>

#include <vector>

class ClazyContext;

using namespace clang;

static bool isQStringBuilder(QualType t)
{
    CXXRecordDecl *record = clazy::typeAsRecord(t);
    return record && clazy::name(record) == "QStringBuilder";
}

AutoUnexpectedQStringBuilder::AutoUnexpectedQStringBuilder(const std::string &name, ClazyContext *context)
    : CheckBase(name, context, Option_CanIgnoreIncludes)
{
}

void AutoUnexpectedQStringBuilder::VisitDecl(Decl *decl)
{
    auto *varDecl = dyn_cast<VarDecl>(decl);
    if (!varDecl) {
        return;
    }

    QualType qualtype = varDecl->getType();
    const Type *type = qualtype.getTypePtrOrNull();
    if (!type || !type->isRecordType() || !dyn_cast<AutoType>(type) || !isQStringBuilder(qualtype)) {
        return;
    }

    std::string replacement = "QString " + clazy::name(varDecl).str();

    if (qualtype.isConstQualified()) {
        replacement = "const " + replacement;
    }

    SourceLocation start = clazy::getLocStart(varDecl);
    SourceLocation end = varDecl->getLocation();
    std::vector<FixItHint> fixits;
    fixits.push_back(clazy::createReplacement({start, end}, replacement));

    emitWarning(clazy::getLocStart(decl), "auto deduced to be QStringBuilder instead of QString. Possible crash.", fixits);
}

void AutoUnexpectedQStringBuilder::VisitStmt(Stmt *stmt)
{
    auto *lambda = dyn_cast<LambdaExpr>(stmt);
    if (!lambda) {
        return;
    }

    CXXMethodDecl *method = lambda->getCallOperator();
    if (!method || !isQStringBuilder(method->getReturnType())) {
        return;
    }

    emitWarning(clazy::getLocStart(stmt), "lambda return type deduced to be QStringBuilder instead of QString. Possible crash.");
}
