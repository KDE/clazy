/*
  This file is part of the clazy static checker.

    Copyright (C) 2017 Sergio Martins <smartins@kde.org>

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

#include "StringUtils.h"
#include "HierarchyUtils.h"
#include "qcolor-from-literal.h"

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
using namespace std;

// TODO: setNameFromString()

static bool handleStringLiteral(const StringLiteral *literal)
{
    if (!literal)
        return false;

    int length = literal->getLength();
    if (length != 4 && length != 7 && length != 9 && length != 13)
        return false;

    llvm::StringRef str = literal->getString();
    if (!str.startswith("#"))
        return false;

    return true;
}

class QColorFromLiteral_Callback
    : public ClazyAstMatcherCallback
{
public:

    QColorFromLiteral_Callback(CheckBase *base)
        : ClazyAstMatcherCallback(base)
    {

    }

    void run(const MatchFinder::MatchResult &result) override
    {
        const StringLiteral *lt = result.Nodes.getNodeAs<StringLiteral>("myLiteral");
        if (handleStringLiteral(lt))
            m_check->emitWarning(lt, "The QColor ctor taking ints is cheaper than the one taking string literals");
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
    auto call = dyn_cast<CXXMemberCallExpr>(stmt);
    if (!call || call->getNumArgs() != 1)
        return;

    string name = clazy::qualifiedMethodName(call);
    if (name != "QColor::setNamedColor")
        return;

    StringLiteral *lt = clazy::getFirstChildOfType2<StringLiteral>(call->getArg(0));
    if (handleStringLiteral(lt))
        emitWarning(lt, "The ctor taking ints is cheaper than QColor::setNamedColor(QString)");
}

void QColorFromLiteral::registerASTMatchers(MatchFinder &finder)
{
    finder.addMatcher(cxxConstructExpr(hasDeclaration(namedDecl(hasName("QColor"))),
                                       hasArgument(0, stringLiteral().bind("myLiteral"))), m_astMatcherCallBack);
}
