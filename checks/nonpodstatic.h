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

#ifndef NON_POD_STATIC_H
#define NON_POD_STATIC_H

#include "checkbase.h"

/**
 * Finds global static non-POD variables.
 *
 * Has some false-positives, like QBasicMutex.
 * You shouldn't fix the cases where they appear in apps instead of libraries.
 */
class NonPodStatic : public CheckBase
{
public:
    NonPodStatic(const std::string &name);
    void VisitDecl(clang::Decl *decl) override;
protected:
    std::vector<std::string> filesToIgnore() const override;
};

#endif
