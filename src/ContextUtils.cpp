/*
    SPDX-FileCopyrightText: 2016 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "ContextUtils.h"
#include "TypeUtils.h"
#include "clazy_stl.h"

#include <clang/AST/Decl.h>
#include <clang/AST/DeclBase.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/DeclFriend.h>
#include <clang/Basic/LLVM.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Basic/Specifiers.h>
#include <llvm/Support/raw_ostream.h>

using namespace clang;

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

static std::string nameForContext(DeclContext *context)
{
    if (auto *ns = dyn_cast<NamespaceDecl>(context)) {
        return ns->getNameAsString();
    }
    if (auto *rec = dyn_cast<CXXRecordDecl>(context)) {
        return rec->getNameAsString();
    } else if (auto *method = dyn_cast<CXXMethodDecl>(context)) {
        return method->getNameAsString();
    } else if (isa<TranslationUnitDecl>(context)) {
        return {};
    } else {
        llvm::errs() << "Unhandled kind: " << context->getDeclKindName() << "\n";
    }

    return {};
}

std::string clazy::getMostNeededQualifiedName(const SourceManager &sourceManager,
                                              CXXMethodDecl *method,
                                              DeclContext *currentScope,
                                              SourceLocation usageLoc,
                                              bool honourUsingDirectives)
{
    if (!currentScope) {
        return method->getQualifiedNameAsString();
    }

    // All namespaces, classes, inner class qualifications
    auto methodContexts = clazy::contextsForDecl(method->getDeclContext());

    // Visible scopes in current scope
    auto visibleContexts = clazy::contextsForDecl(currentScope);

    // Collect using directives
    std::vector<UsingDirectiveDecl *> usings;
    if (honourUsingDirectives) {
        for (DeclContext *context : visibleContexts) {
            for (UsingDirectiveDecl *directive : context->using_directives()) {
                directive->dump();
                usings.push_back(directive);
            }
        }
    }

    for (UsingDirectiveDecl *u : usings) {
        NamespaceDecl *ns = u->getNominatedNamespace();
        if (ns) {
            if (sourceManager.isBeforeInSLocAddrSpace(usageLoc, u->getBeginLoc())) {
                continue;
            }
            visibleContexts.push_back(ns->getFirstDecl());
        }
    }

    for (DeclContext *context : visibleContexts) {
        if (context != method->getParent()) { // Don't remove the most immediate
            auto it = std::ranges::find_if(methodContexts, [context](DeclContext *c) {
                if (c == context) {
                    return true;
                }
                auto *ns1 = dyn_cast<NamespaceDecl>(c);
                auto *ns2 = dyn_cast<NamespaceDecl>(context);
                return ns1 && ns2 && ns1->getQualifiedNameAsString() == ns2->getQualifiedNameAsString();
            });
            if (it != methodContexts.end()) {
                methodContexts.erase(it, it + 1);
            }
        }
    }

    std::string neededContexts;
    for (DeclContext *context : methodContexts) {
        neededContexts = nameForContext(context) + "::" + neededContexts;
    }

    const std::string result = neededContexts + method->getNameAsString();
    return result;
}

bool clazy::canTakeAddressOf(CXXMethodDecl *method, DeclContext *context, bool &isSpecialProtectedCase)
{
    isSpecialProtectedCase = false;
    if (!method || !method->getParent()) {
        return false;
    }

    if (method->getAccess() == clang::AccessSpecifier::AS_public) {
        return true;
    }

    if (!context) {
        return false;
    }

    CXXRecordDecl *contextRecord = nullptr;

    do {
        contextRecord = dyn_cast<CXXRecordDecl>(context);
        context = context->getParent();
    } while (contextRecord == nullptr && context);

    if (!contextRecord) { // If we're not inside a class method we can't take the address of a private/protected method
        return false;
    }

    CXXRecordDecl *record = method->getParent();
    if (record == contextRecord) {
        return true;
    }

    // We're inside a method belonging to a class (contextRecord).
    // Is contextRecord a friend of record ? Lets check:

    for (auto *fr : record->friends()) {
        TypeSourceInfo *si = fr->getFriendType();
        if (si) {
            const Type *t = si->getType().getTypePtrOrNull();
            const CXXRecordDecl *friendClass = t ? t->getAsCXXRecordDecl() : nullptr;
            if (friendClass == contextRecord) {
                return true;
            }
        }
    }

    // There's still hope, lets see if the context is nested inside the class we're trying to access
    // Inner classes can access private members of outer classes.
    DeclContext *it = contextRecord;
    do {
        it = it->getParent();
        if (it == record) {
            return true;
        }
    } while (it);

    if (method->getAccess() == clang::AccessSpecifier::AS_private) {
        return false;
    }

    if (method->getAccess() != clang::AccessSpecifier::AS_protected) { // shouldn't happen, must be protected at this point.
        return false;
    }

    // For protected there's still hope, since record might be a derived or base class
    if (clazy::derivesFrom(record, contextRecord)) {
        return true;
    }

    if (clazy::derivesFrom(contextRecord, record)) {
        isSpecialProtectedCase = true;
        return true;
    }

    return false;
}
