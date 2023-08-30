/*
  This file is part of the clazy static checker.

    SPDX-FileCopyrightText: 2017 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "qcolor-from-literal.h"
#include "HierarchyUtils.h"
#include "StringUtils.h"

#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/Stmt.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/ASTMatchers/ASTMatchersInternal.h>
#include <clang/Basic/LLVM.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Casting.h>

class ClazyContext;

using namespace clang;
using namespace clang::ast_matchers;

// TODO: setNameFromString()

static bool handleStringLiteral(const StringLiteral *literal)
{
    if (!literal) {
        return false;
    }

    int length = literal->getLength();
    if (length != 4 && length != 7 && length != 9 && length != 13) {
        return false;
    }

    llvm::StringRef str = literal->getString();
    return str.startswith("#");
}

class QColorFromLiteral_Callback : public ClazyAstMatcherCallback
{
public:
    QColorFromLiteral_Callback(CheckBase *base)
        : ClazyAstMatcherCallback(base)
    {
    }

    void run(const MatchFinder::MatchResult &result) override
    {
        const auto *lt = result.Nodes.getNodeAs<StringLiteral>("myLiteral");
        if (handleStringLiteral(lt)) {
            m_check->emitWarning(lt, "The QColor ctor taking ints is cheaper than the one taking string literals");
        }
    }
};

QColorFromLiteral::QColorFromLiteral(const std::string &name, ClazyContext *context)
    : CheckBase(name, context, Option_CanIgnoreIncludes)
    , m_astMatcherCallBack(new QColorFromLiteral_Callback(this))
{
}

QColorFromLiteral::~QColorFromLiteral()
{
    delete m_astMatcherCallBack;
}

void QColorFromLiteral::VisitStmt(Stmt *stmt)
{
    auto *call = dyn_cast<CXXMemberCallExpr>(stmt);
    if (!call || call->getNumArgs() != 1) {
        return;
    }

    std::string name = clazy::qualifiedMethodName(call);
    if (name != "QColor::setNamedColor") {
        return;
    }

    auto *lt = clazy::getFirstChildOfType2<StringLiteral>(call->getArg(0));
    if (handleStringLiteral(lt)) {
        emitWarning(lt, "The ctor taking ints is cheaper than QColor::setNamedColor(QString)");
    }
}

void QColorFromLiteral::registerASTMatchers(MatchFinder &finder)
{
    finder.addMatcher(cxxConstructExpr(hasDeclaration(namedDecl(hasName("QColor"))), hasArgument(0, stringLiteral().bind("myLiteral"))), m_astMatcherCallBack);
}
