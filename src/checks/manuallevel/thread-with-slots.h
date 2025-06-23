/*
    SPDX-FileCopyrightText: 2017 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLAZY_THREAD_WITH_SLOTS_H
#define CLAZY_THREAD_WITH_SLOTS_H

#include "checkbase.h"

#include <string>

/**
 * See README-thread-with-slots.md for more info.
 */
class ThreadWithSlots : public CheckBase
{
public:
    explicit ThreadWithSlots(const std::string &name);
    void VisitStmt(clang::Stmt *stmt) override;
    void VisitDecl(clang::Decl *decl) override;

private:
};

#endif
