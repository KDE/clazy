/*
  This file is part of the clazy static checker.

    Copyright (C) 2016 Sergio Martins <smartins@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "ContextUtils.h"
#include "clazy_stl.h"
#include "TypeUtils.h"
#include "SourceCompatibilityHelpers.h"

#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/SourceManager.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/DeclFriend.h>
#include <clang/AST/Decl.h>
#include <clang/AST/DeclBase.h>
#include <clang/Basic/LLVM.h>
#include <clang/Basic/Specifiers.h>
#include <llvm/Support/raw_ostream.h>

using namespace clang;
using namespace std;

std::vector<DeclContext *> clazy::contextsForDecl(DeclContext *currentScope)
{
    std::vector<DeclContext *> decls;
    decls.reserve(20); // jump-start
    while (currentScope) {
        decls.push_back(currentScope);
        currentScope = currentScope->getParent();
    }

    return decls;
}

static string nameForContext(DeclContext *context)
{
    if (auto *ns = dyn_cast<NamespaceDecl>(context)) {
        return ns->getNameAsString();
    } else if (auto rec = dyn_cast<CXXRecordDecl>(context)) {
        return rec->getNameAsString();
    } else if (auto *method = dyn_cast<CXXMethodDecl>(context)) {
        return method->getNameAsString();
    } else if (dyn_cast<TranslationUnitDecl>(context)) {
        return {};
    } else {
        llvm::errs() << "Unhandled kind: " << context->getDeclKindName() << "\n";
    }

    return {};
}

string clazy::getMostNeededQualifiedName(const SourceManager &sourceManager,
                                         CXXMethodDecl *method,
                                         DeclContext *currentScope,
                                         SourceLocation usageLoc, bool honourUsingDirectives)
{
    if (!currentScope)
        return method->getQualifiedNameAsString();

    // All namespaces, classes, inner class qualifications
    auto methodContexts = clazy::contextsForDecl(method->getDeclContext());

    // Visible scopes in current scope
    auto visibleContexts = clazy::contextsForDecl(currentScope);

    // Collect using directives
    vector<UsingDirectiveDecl*> usings;
    if (honourUsingDirectives) {
        for (DeclContext *context : visibleContexts) {
            clazy::append(context->using_directives(), usings);
        }
    }

    for (UsingDirectiveDecl *u : usings) {
        NamespaceDecl *ns = u->getNominatedNamespace();
        if (ns) {
            if (sourceManager.isBeforeInSLocAddrSpace(usageLoc, clazy::getLocStart(u)))
                continue;

            visibleContexts.push_back(ns->getOriginalNamespace());
        }
    }

    for (DeclContext *context : visibleContexts) {

        if (context != method->getParent()) { // Don't remove the most immediate
            auto it = clazy::find_if(methodContexts, [context](DeclContext *c) {
                if (c == context)
                    return true;
                auto ns1 = dyn_cast<NamespaceDecl>(c);
                auto ns2 = dyn_cast<NamespaceDecl>(context);
                return ns1 && ns2 && ns1->getQualifiedNameAsString() == ns2->getQualifiedNameAsString();

            });
            if (it != methodContexts.end()) {
                methodContexts.erase(it, it + 1);
            }
        }
    }

    string neededContexts;
    for (DeclContext *context : methodContexts) {
        neededContexts = nameForContext(context) + "::" + neededContexts;
    }

    const string result = neededContexts + method->getNameAsString();
    return result;
}

bool clazy::canTakeAddressOf(CXXMethodDecl *method, DeclContext *context, bool &isSpecialProtectedCase)
{
    isSpecialProtectedCase = false;
    if (!method || !method->getParent())
        return false;

    if (method->getAccess() == clang::AccessSpecifier::AS_public)
        return true;

    if (!context)
        return false;

    CXXRecordDecl *contextRecord = nullptr;

    do {
        contextRecord = dyn_cast<CXXRecordDecl>(context);
        context = context->getParent();
    } while (contextRecord == nullptr && context);

    if (!contextRecord) // If we're not inside a class method we can't take the address of a private/protected method
        return false;

    CXXRecordDecl *record = method->getParent();
    if (record == contextRecord)
        return true;

    // We're inside a method belonging to a class (contextRecord).
    // Is contextRecord a friend of record ? Lets check:

    for (auto fr : record->friends()) {
        TypeSourceInfo *si = fr->getFriendType();
        if (si) {
            const Type *t = si->getType().getTypePtrOrNull();
            CXXRecordDecl *friendClass = t ? t->getAsCXXRecordDecl() : nullptr;
            if (friendClass == contextRecord) {
                return true;
            }
        }
    }

    // There's still hope, lets see if the context is nested inside the class we're trying to access
    // Inner classes can access private members of outter classes.
    DeclContext *it = contextRecord;
    do {
        it = it->getParent();
        if (it == record)
            return true;
    } while (it);

    if (method->getAccess() == clang::AccessSpecifier::AS_private)
        return false;

    if (method->getAccess() != clang::AccessSpecifier::AS_protected) // shouldnt happen, must be protected at this point.
        return false;

    // For protected there's still hope, since record might be a derived or base class
    if (clazy::derivesFrom(record, contextRecord))
        return true;

    if (clazy::derivesFrom(contextRecord, record)) {
        isSpecialProtectedCase = true;
        return true;
    }

    return false;
}
