/*
    SPDX-FileCopyrightText: 2015 Klarälvdalens Datakonsult AB a KDAB Group company info@kdab.com
    SPDX-FileContributor: Sérgio Martins <sergio.martins@kdab.com>

    SPDX-FileCopyrightText: 2015 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLAZY_LOOP_UTILS_H
#define CLAZY_LOOP_UTILS_H

#include "clazy_stl.h"

#include <clang/AST/Stmt.h>
#include <clang/AST/StmtCXX.h>
#include <llvm/Support/Casting.h>

namespace clang
{
class Stmt;
class SourceManager;
class SourceLocation;
class Expr;
class ParentMap;
class VarDecl;
}

namespace clazy
{
/**
 * Returns the body of a for, range-foor, while or do-while loop
 */
clang::Stmt *bodyFromLoop(clang::Stmt *);

/**
 * Recursively goes through stmt's children and returns true if it finds a "break", "continue" or a "return" stmt
 * All child statements that are on a source code line <
 * If onlyBeforThisLoc is valid, then this function will only return true if the break/return/continue happens before
 */
bool loopCanBeInterrupted(clang::Stmt *loop, const clang::SourceManager &sm, clang::SourceLocation onlyBeforeThisLoc);

/**
 * Returns true if stmt is a for, while or do-while loop
 */
inline bool isLoop(clang::Stmt *stmt)
{
    return llvm::isa<clang::DoStmt>(stmt) || llvm::isa<clang::WhileStmt>(stmt) || llvm::isa<clang::ForStmt>(stmt) || llvm::isa<clang::CXXForRangeStmt>(stmt);
}

/**
 * Returns the container expression for a range-loop or Q_FOREACH
 *
 * Q_FOREACH (auto f, expression) or for (auto i : expression)
 */
clang::Expr *containerExprForLoop(clang::Stmt *loop);

/**
 * Returns the container decl for a range-loop or Q_FOREACH
 *
 * Q_FOREACH (auto f, container) or for (auto i : container)
 */
clang::VarDecl *containerDeclForLoop(clang::Stmt *loop);

/**
 * Returns true of stmt is inside a for, while or do-while loop.
 * If yes, returns the loop statement, otherwise nullptr.
 */
clang::Stmt *isInLoop(clang::ParentMap *pmap, clang::Stmt *stmt);
}

#endif
