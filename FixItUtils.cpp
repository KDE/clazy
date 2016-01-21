/*
  This file is part of the clang-lazy static checker.

  Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
  Author: Sérgio Martins <sergio.martins@kdab.com>

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

#include "FixItUtils.h"
#include "checkmanager.h"

#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>
#include <clang/Basic/Diagnostic.h>
#include <clang/Lex/Lexer.h>

using namespace FixItUtils;
using namespace clang;
using namespace std;

clang::FixItHint FixItUtils::createReplacement(const clang::SourceRange &range, const std::string &replacement)
{
    if (range.getBegin().isInvalid()) {
        return {};
    } else {
        return FixItHint::CreateReplacement(range, replacement);
    }
}

clang::FixItHint FixItUtils::createInsertion(const clang::SourceLocation &start, const std::string &insertion)
{
    if (start.isInvalid()) {
        return {};
    } else {
        return FixItHint::CreateInsertion(start, insertion);
    }
}

SourceRange FixItUtils::rangeForLiteral(const CompilerInstance& ci, StringLiteral *lt)
{
    if (!lt)
        return {};

    const int numTokens = lt->getNumConcatenated();
    const SourceLocation lastTokenLoc = lt->getStrTokenLoc(numTokens - 1);
    if (lastTokenLoc.isInvalid()) {
        return {};
    }

    SourceRange range;
    range.setBegin(lt->getLocStart());

    SourceLocation end = Lexer::getLocForEndOfToken(lastTokenLoc, 0,
                                                    ci.getSourceManager(),
                                                    ci.getLangOpts()); // For some reason lt->getLocStart() is == to lt->getLocEnd()

    if (!end.isValid()) {
        return {};
    }

    range.setEnd(end);
    return range;
}

void FixItUtils::insertParentMethodCall(const std::string &method, const SourceRange &range, std::vector<FixItHint> &fixits)
{
    fixits.push_back(FixItUtils::createInsertion(range.getEnd(), ")"));
    fixits.push_back(FixItUtils::createInsertion(range.getBegin(), method + '('));
}

bool FixItUtils::insertParentMethodCallAroundStringLiteral(const CompilerInstance& ci, const std::string &method, StringLiteral *lt, std::vector<FixItHint> &fixits)
{
    if (!lt)
        return false;

    const SourceRange range = rangeForLiteral(ci, lt);
    if (range.isInvalid())
        return false;

    insertParentMethodCall(method, range, /*by-ref*/fixits);
    return true;
}

SourceLocation FixItUtils::locForNextToken(const CompilerInstance &ci, SourceLocation start, tok::TokenKind kind)
{
    if (!start.isValid())
        return {};

    Token result;
    Lexer::getRawToken(start, result, ci.getSourceManager(), ci.getLangOpts());

    if (result.getKind() == kind)
        return start;


    auto nextStart = Lexer::getLocForEndOfToken(start, 0, ci.getSourceManager(), ci.getLangOpts());
    if (nextStart.getRawEncoding() == start.getRawEncoding())
        return {};

    return locForNextToken(ci, nextStart, kind);
}

SourceLocation FixItUtils::biggestSourceLocationInStmt(const SourceManager &sm, Stmt *stmt)
{
    if (!stmt)
        return {};

    SourceLocation biggestLoc = stmt->getLocEnd();

    for (auto it = stmt->child_begin(), end = stmt->child_end(); it != end; ++it) {
        SourceLocation candidateLoc = biggestSourceLocationInStmt(sm, *it);
        if (candidateLoc.isValid() && sm.isBeforeInSLocAddrSpace(biggestLoc, candidateLoc))
            biggestLoc = candidateLoc;
    }

    return biggestLoc;
}

SourceLocation FixItUtils::locForEndOfToken(const CompilerInstance &ci, SourceLocation start, int offset)
{
    return Lexer::getLocForEndOfToken(start, offset, ci.getSourceManager(), ci.getLangOpts());
}

bool FixItUtils::transformTwoCallsIntoOne(const CompilerInstance &ci, CallExpr *call1, CXXMemberCallExpr *call2,
                                          const string &replacement, vector<FixItHint> &fixits)
{
    Expr *implicitArgument = call2->getImplicitObjectArgument();
    if (!implicitArgument)
        return false;

    const SourceLocation start1 = call1->getLocStart();
    const SourceLocation end1 = FixItUtils::locForEndOfToken(ci, start1, -1); // -1 of offset, so we don't need to insert '('
    if (end1.isInvalid())
        return false;

    const SourceLocation start2 = implicitArgument->getLocEnd();
    const SourceLocation end2 = call2->getLocEnd();
    if (start2.isInvalid() || end2.isInvalid())
        return false;

    // qgetenv("foo").isEmpty()
    // ^                         start1
    //       ^                   end1
    //              ^            start2
    //                        ^  end2
    fixits.push_back(FixItUtils::createReplacement({ start1, end1 }, replacement));
    fixits.push_back(FixItUtils::createReplacement({ start2, end2 }, ")"));

    return true;
}

bool FixItUtils::transformTwoCallsIntoOneV2(const CompilerInstance &ci, CXXMemberCallExpr *call2, const string &replacement, std::vector<FixItHint> &fixits)
{
    Expr *implicitArgument = call2->getImplicitObjectArgument();
    if (!implicitArgument)
        return false;

    SourceLocation start = implicitArgument->getLocStart();
    start = FixItUtils::locForEndOfToken(ci, start, 0);
    const SourceLocation end = call2->getLocEnd();
    if (start.isInvalid() || end.isInvalid())
        return false;

    fixits.push_back(FixItUtils::createReplacement({ start, end }, replacement));
    return true;
}
