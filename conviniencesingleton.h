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

#ifndef CONV_SINGLETON_H
#define CONV_SINGLETON_H

namespace clang {
class SourceManager;
}

class ConvinienceSingleton
{
public:
    static ConvinienceSingleton * instance();
    clang::SourceManager *sm;
private:
    ConvinienceSingleton();
};

#endif
