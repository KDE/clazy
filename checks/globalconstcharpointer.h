/**********************************************************************
**  Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
**  Author: Sérgio Martins <sergio.martins@kdab.com>
**
** This file may be distributed and/or modified under the terms of the
** GNU Lesser General Public License version 2.1 and version 3 as published by the
** Free Software Foundation and appearing in the file LICENSE.LGPL.txt included.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**********************************************************************/

#ifndef GLOBAL_CONST_CHAR_POINTER_H
#define GLOBAL_CONST_CHAR_POINTER_H

#include "checkbase.h"

#include <vector>

/**
 * Finds where you're using const char *foo; instead of const char *const foo; or const char []foo;
 * The first case adds a pointer in .data, pointing to .rodata, the other cases only use .rodata
 */
class GlobalConstCharPointer : public CheckBase
{
public:
    explicit GlobalConstCharPointer(clang::CompilerInstance &ci);
    void VisitDecl(clang::Decl *decl) override;
    std::string name() const override;
    std::vector<std::string> filesToIgnore() const override;
};

#endif
