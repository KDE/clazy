/*
    This file is part of the clazy static checker.

    SPDX-FileCopyrightText: 2015 Klarälvdalens Datakonsult AB a KDAB Group company info@kdab.com
    Author: Sérgio Martins <sergio.martins@kdab.com>

    SPDX-FileCopyrightText: 2015-2016 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef TEMPORARY_ITERATOR_H
#define TEMPORARY_ITERATOR_H

#include "checkbase.h"

#include <llvm/ADT/StringRef.h>

#include <map>
#include <string>
#include <vector>

class ClazyContext;
namespace clang
{
class Stmt;
} // namespace clang

/**
 * Finds places where you're using iterators on temporary containers.
 *
 * For example getList().constBegin(), getList().constEnd() would provoke a crash when dereferencing
 * the iterator.
 */
class TemporaryIterator : public CheckBase
{
public:
    TemporaryIterator(const std::string &name, ClazyContext *context);
    void VisitStmt(clang::Stmt *stm) override;

private:
    std::map<llvm::StringRef, std::vector<llvm::StringRef>> m_methodsByType;
};

#endif
