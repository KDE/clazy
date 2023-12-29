/*
    SPDX-FileCopyrightText: 2015 Klarälvdalens Datakonsult AB a KDAB Group company info@kdab.com
    Author: Sérgio Martins <sergio.martins@kdab.com>

    SPDX-FileCopyrightText: 2015 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef DETACHING_TEMPORARIES_H
#define DETACHING_TEMPORARIES_H

#include "checks/detachingbase.h"

#include <llvm/ADT/StringRef.h>

#include <map>
#include <string>
#include <vector>

class ClazyContext;
namespace clang
{
class CXXMethodDecl;
class Stmt;
} // namespace clang

/**
 * Finds places where you're calling non-const member functions on temporaries.
 *
 * For example getList().first(), which would detach if the container is shared.
 * See README-deatching-temporary for more information
 */
class DetachingTemporary : public DetachingBase
{
public:
    DetachingTemporary(const std::string &name, ClazyContext *context);
    void VisitStmt(clang::Stmt *stm) override;

private:
    bool isDetachingMethod(clang::CXXMethodDecl *method) const;
    std::map<llvm::StringRef, std::vector<llvm::StringRef>> m_writeMethodsByType;
};

#endif
