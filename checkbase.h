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

#ifndef CHECK_BASE_H
#define CHECK_BASE_H

#include <clang/Frontend/CompilerInstance.h>

namespace clang {
class CXXMethodDecl;
class Stmt;
class Decl;
class ParentMap;
class TranslationUnitDecl;
class FixItHint;
}

class CheckBase
{
public:
    explicit CheckBase(clang::CompilerInstance &m_ci);
    ~CheckBase();

    void VisitStatement(clang::Stmt *stm);
    void VisitDeclaration(clang::Decl *stm);

    virtual std::string name() const = 0;

protected:
    virtual void VisitStmt(clang::Stmt *stm);
    virtual void VisitDecl(clang::Decl *decl);
    bool shouldIgnoreFile(const std::string &filename) const;
    virtual std::vector<std::string> filesToIgnore() const;
    void emitWarning(clang::SourceLocation loc, const char *error) const;
    void emitWarning(clang::SourceLocation loc, std::string error) const;
    void emitWarning(clang::SourceLocation loc, std::string error, const std::vector<clang::FixItHint> &fixits) const;

    clang::CompilerInstance &m_ci;
    clang::TranslationUnitDecl *m_tu;
    clang::ParentMap *m_parentMap;
    clang::CXXMethodDecl *m_lastMethodDecl;
};

#endif
