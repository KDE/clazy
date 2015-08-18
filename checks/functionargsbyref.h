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

#ifndef FUNCTION_ARGS_BY_REF_H
#define FUNCTION_ARGS_BY_REF_H

#include "checkbase.h"

namespace clang {
class Decl;
}

/**
 * Finds functions where big non-trivial types are passed by value instead of const-ref.
 * Looks into the body of the functions to see if the argument are read-only, it doesn't emit a warning otherwise.
 */
class FunctionArgsByRef : public CheckBase
{
public:
    FunctionArgsByRef(const std::string &name);
    void VisitDecl(clang::Decl *decl) override;
protected:
    std::vector<std::string> filesToIgnore() const override;
};

#endif
