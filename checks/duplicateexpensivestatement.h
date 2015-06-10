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

#ifndef DUPLICATE_EXPENSIVE_STATEMENT_H
#define DUPLICATE_EXPENSIVE_STATEMENT_H

#include "checkbase.h"

namespace clang {
class FunctionDecl;
class ValueDecl;
}

class DuplicateExpensiveStatement : public CheckBase {
public:
    explicit DuplicateExpensiveStatement(clang::CompilerInstance &ci);
    void VisitDecl(clang::Decl *decl) override;
    std::string name() const override;
private:
    void inspectStatement(clang::Stmt *stm);
    std::map<clang::FunctionDecl*, std::map<clang::ValueDecl*, int> > m_expensiveCounts;
    clang::FunctionDecl *m_currentFunctionDecl = nullptr;
};



#endif
