/*
    SPDX-FileCopyrightText: 2025 Alexander Lohnau <alexander.lohnau@gmx.de>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "ClazyVisitHelper.h"
#include "AccessSpecifierManager.h"
#include "Utils.h"
#include "clang/AST/ParentMap.h"

namespace clazy::VisitHelper
{

using namespace clang;

bool VisitDecl(Decl *decl, ClazyContext *context, const std::vector<CheckBase *> &checksToVisit, const std::vector<CheckBase *> &checksToVisitAllTypedefs)
{
    if (AccessSpecifierManager *a = context->accessSpecifierManager) { // Needs to visit system headers too (qobject.h for example)
        a->VisitDeclaration(decl);
    }

    const SourceLocation locStart = decl->getBeginLoc();
    if (locStart.isInvalid() || context->sm.isInSystemHeader(locStart)) {
        if (isa<TypedefNameDecl>(decl)) {
            for (CheckBase *check : checksToVisitAllTypedefs) {
                check->VisitDecl(decl);
            }
        }
        return true;
    }

    const bool isFromIgnorableInclude = context->ignoresIncludedFiles() && !Utils::isMainFile(context->sm, locStart);

    context->lastDecl = decl;

    if (auto *fdecl = dyn_cast<FunctionDecl>(decl)) {
        context->lastFunctionDecl = fdecl;
        if (auto *mdecl = dyn_cast<CXXMethodDecl>(fdecl)) {
            context->lastMethodDecl = mdecl;
        }
    }

    for (CheckBase *check : checksToVisit) {
        if (!(isFromIgnorableInclude && check->canIgnoreIncludes())) {
            check->VisitDecl(decl);
        }
    }

    return true;
}

bool VisitStmt(Stmt *stmt, ClazyContext *context, const std::vector<CheckBase *> &checksToVisit)
{
    const SourceLocation locStart = stmt->getBeginLoc();
    if (locStart.isInvalid() || context->sm.isInSystemHeader(locStart)) {
        return true;
    }

    if (!context->parentMap) {
        if (context->astContext->getDiagnostics().hasUnrecoverableErrorOccurred()) {
            return false; // ParentMap sometimes crashes when there were errors. Doesn't like a botched AST.
        }

        context->parentMap = new ParentMap(stmt);
    }

    // clang::ParentMap takes a root statement, but there's no root statement in the AST, the root is a declaration
    // So add to parent map each time we go into a different hierarchy
    if (!context->parentMap->hasParent(stmt)) {
        context->parentMap->addStmt(stmt);
    }

    const bool isFromIgnorableInclude = context->ignoresIncludedFiles() && !Utils::isMainFile(context->sm, locStart);
    for (CheckBase *check : checksToVisit) {
        if (!(isFromIgnorableInclude && check->canIgnoreIncludes())) {
            check->VisitStmt(stmt);
        }
    }

    return true;
}

}
