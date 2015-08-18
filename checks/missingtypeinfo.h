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

#ifndef MORE_WARNINGS_MOVABLE_H
#define MORE_WARNINGS_MOVABLE_H

#include "checkbase.h"

namespace clang {
class ClassTemplateSpecializationDecl;
}

/**
 * Advises usage of Q_PRIMITIVE_TYPE in cases where you're using QList<T> and sizeof(T) > sizeof(void*)
 * or using QVector<T>. Unless they already have this classification.
 */
class MissingTypeinfo : public CheckBase
{
public:
    MissingTypeinfo(const std::string &name);
    void VisitDecl(clang::Decl *decl) override;
private:
    void registerQTypeInfo(clang::ClassTemplateSpecializationDecl *decl);
    bool ignoreTypeInfo(const std::string &className) const;
    std::set<std::string> m_typeInfos;
};

#endif
