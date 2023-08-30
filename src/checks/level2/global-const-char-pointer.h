/*
    This file is part of the clazy static checker.

    SPDX-FileCopyrightText: 2015 Klarälvdalens Datakonsult AB a KDAB Group company info@kdab.com
    Author: Sérgio Martins <sergio.martins@kdab.com>

    SPDX-FileCopyrightText: 2015 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef GLOBAL_CONST_CHAR_POINTER_H
#define GLOBAL_CONST_CHAR_POINTER_H

#include "checkbase.h"

#include <string>
#include <vector>

class ClazyContext;
namespace clang
{
class Decl;
} // namespace clang

/**
 * Finds where you're using const char *foo; instead of const char *const foo; or const char []foo;
 * The first case adds a pointer in .data, pointing to .rodata, the other cases only use .rodata
 */
class GlobalConstCharPointer : public CheckBase
{
public:
    GlobalConstCharPointer(const std::string &name, ClazyContext *context);
    void VisitDecl(clang::Decl *decl) override;
};

#endif
