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

#include "qcolor-from-literal.h"
#include "checkmanager.h"

using namespace clang;
using namespace clang::ast_matchers;
using namespace std;

// TODO: setNameFromString()

static void handleStringLiteral(const StringLiteral *literal, CheckBase *check)
{
    if (!literal)
        return;

    int length = literal->getLength();
    if (length != 4 && length != 7 && length != 9 && length != 13)
        return;

    llvm::StringRef str = literal->getString();
    if (!str.startswith("#"))
        return;

    check->emitWarning(literal, "The QColor ctor taking ints is much cheaper than the one taking string literals");
}

class QColorFromLiteral_Callback : public ClazyAstMatcherCallback
{
public :

    QColorFromLiteral_Callback(CheckBase *base)
        : ClazyAstMatcherCallback(base)
    {

    }

    void run(const MatchFinder::MatchResult &result) override
    {
        handleStringLiteral(result.Nodes.getNodeAs<StringLiteral>("myLiteral"), m_check);
    }
};


QColorFromLiteral::QColorFromLiteral(const std::string &name, ClazyContext *context)
    : CheckBase(name, context)
    , m_astMatcherCallBack(new QColorFromLiteral_Callback(this))
{
}

QColorFromLiteral::~QColorFromLiteral()
{
    delete m_astMatcherCallBack;
}

void QColorFromLiteral::registerASTMatchers(MatchFinder &finder)
{
    finder.addMatcher(cxxConstructExpr(hasDeclaration(namedDecl(hasName("QColor"))),
                                       hasArgument(0, stringLiteral().bind("myLiteral"))), m_astMatcherCallBack);
}

REGISTER_CHECK("qcolor-from-literal", QColorFromLiteral, CheckLevel0)
