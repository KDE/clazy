/*
    SPDX-FileCopyrightText: 2015 Klarälvdalens Datakonsult AB a KDAB Group company info@kdab.com
    SPDX-FileContributor: Sérgio Martins <sergio.martins@kdab.com>

    SPDX-FileCopyrightText: 2015 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "non-pod-global-static.h"
#include "ClazyContext.h"
#include "QtUtils.h"
#include "clazy_stl.h"

#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/Stmt.h>
#include <clang/Basic/LLVM.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/Specifiers.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Lex/Lexer.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Casting.h>

#include <vector>

using namespace clang;

static bool shouldIgnoreType(StringRef name)
{
    static std::vector<StringRef> blacklist = {"Holder", "AFUNC", "QLoggingCategory", "QThreadStorage", "QQmlModuleRegistration"};
    return clazy::contains(blacklist, name);
}

NonPodGlobalStatic::NonPodGlobalStatic(const std::string &name, ClazyContext *context)
    : CheckBase(name, context, Option_CanIgnoreIncludes)
{
    m_filesToIgnore = {"main.cpp", "qrc_", "qdbusxml2cpp"};
}

void NonPodGlobalStatic::VisitStmt(clang::Stmt *stm)
{
    VarDecl *varDecl = m_context->lastDecl ? dyn_cast<VarDecl>(m_context->lastDecl) : nullptr;
    if (!varDecl || varDecl->isConstexpr() || varDecl->isExternallyVisible() || !varDecl->isFileVarDecl()) {
        return;
    }

    if (shouldIgnoreFile(stm->getBeginLoc())) {
        return;
    }

    StorageDuration sd = varDecl->getStorageDuration();
    if (sd != StorageDuration::SD_Static) {
        return;
    }

    const SourceLocation declStart = varDecl->getBeginLoc();

    if (declStart.isMacroID()) {
        auto macroName = static_cast<std::string>(Lexer::getImmediateMacroName(declStart, sm(), lo()));
        if (clazy::startsWithAny(macroName, {"Q_IMPORT_PLUGIN", "Q_CONSTRUCTOR_FUNCTION", "Q_DESTRUCTOR_FUNCTION"})) { // Don't warn on these
            return;
        }
    }

    auto *ctorExpr = dyn_cast<CXXConstructExpr>(stm);
    if (!ctorExpr) {
        return;
    }

    auto *ctorDecl = ctorExpr->getConstructor();
    auto *recordDecl = ctorDecl ? ctorDecl->getParent() : nullptr;
    if (!recordDecl) {
        return;
    }

    if (recordDecl->hasTrivialDestructor()) {
        // Has a trivial dtor, but now lets check the ctors.

        if (ctorDecl->isDefaultConstructor() && recordDecl->hasTrivialDefaultConstructor()) {
            // both dtor and called ctor are trivial, no warning
            return;
        }
        if (ctorDecl->isConstexpr()) {
            // Used ctor is constexpr, it's fine
            return;
        }
    }

    if (m_context->isQtDeveloper() && clazy::isBootstrapping(m_context->getPreprocessorOpts())) {
        return;
    }

    StringRef className = clazy::name(recordDecl);
    if (!shouldIgnoreType(className)) {
        const std::string varName = varDecl->getQualifiedNameAsString();
        const std::string error = std::string("non-POD static (") + className.data() + " " + varName + std::string(")");
        emitWarning(declStart, error);
    }
}
