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

#ifndef QMAP_KEY_CHECK_H
#define QMAP_KEY_CHECK_H

#include "checkbase.h"

/**
 * Finds cases where you're using QMap<K,T> and K is a pointer. QHash<K,T> should be used instead.
 */
class QMapKeyChecker : public CheckBase {
public:
    QMapKeyChecker(const std::string &name);
    void VisitDecl(clang::Decl *decl) override;
};

#endif
