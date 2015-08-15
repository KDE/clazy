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

#include "nonpodstatic.h"
#include "Utils.h"

#include <clang/AST/DeclCXX.h>

using namespace clang;
using namespace std;

static bool shouldIgnoreType(const std::string &name)
{
    // Q_GLOBAL_STATIC and such
    static vector<string> blacklist = {"Holder", "AFUNC", "QLoggingCategory"};
    return find(blacklist.cbegin(), blacklist.cend(), name) != blacklist.cend();
}

NonPodStatic::NonPodStatic(clang::CompilerInstance &ci)
    : CheckBase(ci)
{
}

void NonPodStatic::VisitDecl(clang::Decl *decl)
{
    auto varDecl = dyn_cast<VarDecl>(decl);
    if (!varDecl || varDecl->isConstexpr() || varDecl->isExternallyVisible() || !varDecl->isFileVarDecl())
        return;

    StorageDuration sd = varDecl->getStorageDuration();
    if (sd != StorageDuration::SD_Static)
        return;

    QualType qt = varDecl->getType();
    const bool isTrivial = qt.isTrivialType(m_ci.getASTContext());
    if (isTrivial)
        return;

    SourceManager &sm = m_ci.getSourceManager();
    std::string filename = sm.getFilename(decl->getLocStart());
    if (filename.empty())
        filename = decl->getLocation().printToString(sm);

    if (CheckBase::shouldIgnoreFile(filename))
        return;

    const Type *t = qt.getTypePtrOrNull();
    if (t == nullptr || t->getAsCXXRecordDecl() == nullptr || Utils::hasConstexprCtor(t->getAsCXXRecordDecl()))
        return;

    if (!shouldIgnoreType(t->getAsCXXRecordDecl()->getName())) {
        std::string error = "non-POD static";
        emitWarning(decl->getLocStart(), error.c_str());
    }
}

std::vector<string> NonPodStatic::filesToIgnore() const
{
    return {"main.cpp"};
}

std::string NonPodStatic::name() const
{
    return "non-pod-global-static";
}
