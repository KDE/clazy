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

#ifndef CLAZY_FIXIT_UTILS_H
#define CLAZY_FIXIT_UTILS_H

#include <clang/Parse/Parser.h>
#include <clang/Basic/TokenKinds.h>

#include <string>
#include <vector>

namespace clang {
class ASTContext;
class FixItHint;
class SourceManager;
class SourceRange;
class SourceLocation;
class StringLiteral;
class CallExpr;
class CXXMemberCallExpr;
class Stmt;
}

namespace clazy {

/**
 * Replaces whatever is in range, with replacement
 */
clang::FixItHint createReplacement(clang::SourceRange range, const std::string &replacement);

/**
 * Inserts insertion at start
 */
clang::FixItHint createInsertion(clang::SourceLocation start, const std::string &insertion);

/**
 * Transforms foo into method(foo), by inserting "method(" at the beginning, and ')' at the end
 */
void insertParentMethodCall(const std::string &method, clang::SourceRange range, std::vector<clang::FixItHint> &fixits);

/**
 * Transforms foo into method("literal"), by inserting "method(" at the beginning, and ')' at the end
 * Takes into account multi-token literals such as "foo""bar"
 */
bool insertParentMethodCallAroundStringLiteral(const clang::ASTContext *context, const std::string &method, clang::StringLiteral *lt, std::vector<clang::FixItHint> &fixits);

/**
 * Returns the range this literal spans. Takes into account multi token literals, such as "foo""bar"
 */
clang::SourceRange rangeForLiteral(const clang::ASTContext *context, clang::StringLiteral *);

/**
 * Goes through all children of stmt and finds the biggests source location.
 */
clang::SourceLocation biggestSourceLocationInStmt(const clang::SourceManager &sm, clang::Stmt *stmt);

clang::SourceLocation locForNextToken(const clang::ASTContext *context, clang::SourceLocation start, clang::tok::TokenKind kind);

/**
 * Returns the end location of the token that starts at start.
 *
 * For example, having this expr:
 * getenv("FOO")
 *
 * ^              // expr->getLocStart()
 *             ^  // expr->getLocEnd()
 *      ^         // clazy::locForEndOfToken(expr->getLocStart())
 */
clang::SourceLocation locForEndOfToken(const clang::ASTContext *context, clang::SourceLocation start, int offset = 0);

/**
 * Transforms a call such as: foo("hello").bar() into baz("hello")
 */
bool transformTwoCallsIntoOne(const clang::ASTContext *context, clang::CallExpr *foo, clang::CXXMemberCallExpr *bar,
                              const std::string &baz, std::vector<clang::FixItHint> &fixits);


/**
 * Transforms a call such as: foo("hello").bar() into baz()
 * This version basically replaces everything from start to end with baz.
 */
bool transformTwoCallsIntoOneV2(const clang::ASTContext *context, clang::CXXMemberCallExpr *bar,
                                const std::string &baz, std::vector<clang::FixItHint> &fixits);

clang::FixItHint fixItReplaceWordWithWord(const clang::ASTContext *context, clang::Stmt *begin,
                                          const std::string &replacement, const std::string &replacee);

std::vector<clang::FixItHint> fixItRemoveToken(const clang::ASTContext *context,
                                               clang::Stmt *stmt,
                                               bool removeParenthesis);

}

#endif

