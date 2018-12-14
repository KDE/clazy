/*
  This file is part of the clazy static checker.

    Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
    Author: Sérgio Martins <sergio.martins@kdab.com>

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

#include "FixItUtils.h"
#include "StringUtils.h"
#include "SourceCompatibilityHelpers.h"

#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>
#include <clang/Basic/Diagnostic.h>
#include <clang/Lex/Lexer.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/Stmt.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Lex/Token.h>

using namespace clazy;
using namespace clang;
using namespace std;

clang::FixItHint clazy::createReplacement(clang::SourceRange range, const std::string &replacement)
{
    if (range.getBegin().isInvalid()) {
        return {};
    } else {
        return FixItHint::CreateReplacement(range, replacement);
    }
}

clang::FixItHint clazy::createInsertion(clang::SourceLocation start, const std::string &insertion)
{
    if (start.isInvalid()) {
        return {};
    } else {
        return FixItHint::CreateInsertion(start, insertion);
    }
}

SourceRange clazy::rangeForLiteral(const ASTContext *context, StringLiteral *lt)
{
    if (!lt)
        return {};

    const int numTokens = lt->getNumConcatenated();
    const SourceLocation lastTokenLoc = lt->getStrTokenLoc(numTokens - 1);
    if (lastTokenLoc.isInvalid()) {
        return {};
    }

    SourceRange range;
    range.setBegin(clazy::getLocStart(lt));

    SourceLocation end = Lexer::getLocForEndOfToken(lastTokenLoc, 0,
                                                    context->getSourceManager(),
                                                    context->getLangOpts()); // For some reason getLocStart(lt) is == to getLocEnd(lt)

    if (!end.isValid()) {
        return {};
    }

    range.setEnd(end);
    return range;
}

void clazy::insertParentMethodCall(const std::string &method, SourceRange range, std::vector<FixItHint> &fixits)
{
    fixits.push_back(clazy::createInsertion(range.getEnd(), ")"));
    fixits.push_back(clazy::createInsertion(range.getBegin(), method + '('));
}

bool clazy::insertParentMethodCallAroundStringLiteral(const ASTContext *context,
                                                      const std::string &method,
                                                      StringLiteral *lt,
                                                      std::vector<FixItHint> &fixits)
{
    if (!lt)
        return false;

    const SourceRange range = rangeForLiteral(context, lt);
    if (range.isInvalid())
        return false;

    insertParentMethodCall(method, range, /*by-ref*/ fixits);
    return true;
}

SourceLocation clazy::locForNextToken(const ASTContext *context, SourceLocation start, tok::TokenKind kind)
{
    if (!start.isValid())
        return {};

    Token result;
    Lexer::getRawToken(start, result, context->getSourceManager(), context->getLangOpts());

    if (result.getKind() == kind)
        return start;

    auto nextStart = Lexer::getLocForEndOfToken(start, 0, context->getSourceManager(), context->getLangOpts());
    if (nextStart.getRawEncoding() == start.getRawEncoding())
        return {};

    return locForNextToken(context, nextStart, kind);
}

SourceLocation clazy::biggestSourceLocationInStmt(const SourceManager &sm, Stmt *stmt)
{
    if (!stmt)
        return {};

    SourceLocation biggestLoc = getLocEnd(stmt);

    for (auto child : stmt->children()) {
        SourceLocation candidateLoc = biggestSourceLocationInStmt(sm, child);
        if (candidateLoc.isValid() && sm.isBeforeInSLocAddrSpace(biggestLoc, candidateLoc))
            biggestLoc = candidateLoc;
    }

    return biggestLoc;
}

SourceLocation clazy::locForEndOfToken(const ASTContext *context, SourceLocation start, int offset)
{
    return Lexer::getLocForEndOfToken(start, offset, context->getSourceManager(), context->getLangOpts());
}

bool clazy::transformTwoCallsIntoOne(const ASTContext *context, CallExpr *call1, CXXMemberCallExpr *call2,
                                     const string &replacement, vector<FixItHint> &fixits)
{
    Expr *implicitArgument = call2->getImplicitObjectArgument();
    if (!implicitArgument)
        return false;

    const SourceLocation start1 = clazy::getLocStart(call1);
    const SourceLocation end1 = clazy::locForEndOfToken(context, start1, -1); // -1 of offset, so we don't need to insert '('
    if (end1.isInvalid())
        return false;

    const SourceLocation start2 = getLocEnd(implicitArgument);
    const SourceLocation end2 = getLocEnd(call2);
    if (start2.isInvalid() || end2.isInvalid())
        return false;

    // qgetenv("foo").isEmpty()
    // ^                         start1
    //       ^                   end1
    //              ^            start2
    //                        ^  end2
    fixits.push_back(clazy::createReplacement({ start1, end1 }, replacement));
    fixits.push_back(clazy::createReplacement({ start2, end2 }, ")"));

    return true;
}

bool clazy::transformTwoCallsIntoOneV2(const ASTContext *context, CXXMemberCallExpr *call2,
                                       const string &replacement, std::vector<FixItHint> &fixits)
{
    Expr *implicitArgument = call2->getImplicitObjectArgument();
    if (!implicitArgument)
        return false;

    SourceLocation start = clazy::getLocStart(implicitArgument);
    start = clazy::locForEndOfToken(context, start, 0);
    const SourceLocation end = getLocEnd(call2);
    if (start.isInvalid() || end.isInvalid())
        return false;

    fixits.push_back(clazy::createReplacement({ start, end }, replacement));
    return true;
}

FixItHint clazy::fixItReplaceWordWithWord(const ASTContext *context, clang::Stmt *begin,
                                          const string &replacement, const string &replacee)
{
    auto &sm = context->getSourceManager();
    SourceLocation rangeStart = clazy::getLocStart(begin);
    SourceLocation rangeEnd = Lexer::getLocForEndOfToken(rangeStart, -1, sm, context->getLangOpts());

    if (rangeEnd.isInvalid()) {
        // Fallback. Have seen a case in the wild where the above would fail, it's very rare
        rangeEnd = rangeStart.getLocWithOffset(replacee.size() - 2);
        if (rangeEnd.isInvalid()) {
            clazy::printLocation(sm, rangeStart);
            clazy::printLocation(sm, rangeEnd);
            clazy::printLocation(sm, Lexer::getLocForEndOfToken(rangeStart, 0, sm, context->getLangOpts()));
            return {};
        }
    }

    return FixItHint::CreateReplacement(SourceRange(rangeStart, rangeEnd), replacement);
}

vector<FixItHint> clazy::fixItRemoveToken(const ASTContext *context, Stmt *stmt, bool removeParenthesis)
{
    SourceLocation start = clazy::getLocStart(stmt);
    SourceLocation end = Lexer::getLocForEndOfToken(start, removeParenthesis ? 0 : -1,
                                                    context->getSourceManager(), context->getLangOpts());

    vector<FixItHint> fixits;

    if (start.isValid() && end.isValid()) {
        fixits.push_back(FixItHint::CreateRemoval(SourceRange(start, end)));

        if (removeParenthesis) {
            // Remove the last parenthesis
            fixits.push_back(FixItHint::CreateRemoval(SourceRange(clazy::getLocEnd(stmt), clazy::getLocEnd(stmt))));
        }
    }

    return fixits;
}
