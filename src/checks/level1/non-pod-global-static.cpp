/*
    This file is part of the clazy static checker.

    Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
    Author: Sérgio Martins <sergio.martins@kdab.com>

    Copyright (C) 2015 Sergio Martins <smartins@kde.org>

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

#include "non-pod-global-static.h"
#include "QtUtils.h"
#include "ClazyContext.h"
#include "SourceCompatibilityHelpers.h"
#include "clazy_stl.h"

#include <clang/AST/DeclCXX.h>
#include <clang/Lex/Lexer.h>
#include <clang/AST/Decl.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/Stmt.h>
#include <clang/Basic/LLVM.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/Specifiers.h>
#include <clang/Frontend/CompilerInstance.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Casting.h>

#include <vector>

using namespace clang;
using namespace std;

static bool shouldIgnoreType(StringRef name)
{
    // Q_GLOBAL_STATIC and such
    static vector<StringRef> blacklist = {"Holder", "AFUNC", "QLoggingCategory", "QThreadStorage"};
    return clazy::contains(blacklist, name);
}

NonPodGlobalStatic::NonPodGlobalStatic(const std::string &name, ClazyContext *context)
    : CheckBase(name, context, Option_CanIgnoreIncludes)
{
    m_filesToIgnore = { "main.cpp", "qrc_", "qdbusxml2cpp" };
}

void NonPodGlobalStatic::VisitStmt(clang::Stmt *stm)
{
    VarDecl *varDecl = m_context->lastDecl ? dyn_cast<VarDecl>(m_context->lastDecl) : nullptr;
    if (!varDecl || varDecl->isConstexpr() || varDecl->isExternallyVisible() || !varDecl->isFileVarDecl())
        return;

    if (shouldIgnoreFile(clazy::getLocStart(stm)))
        return;

    StorageDuration sd = varDecl->getStorageDuration();
    if (sd != StorageDuration::SD_Static)
        return;

    const SourceLocation declStart = clazy::getLocStart(varDecl);

    if (declStart.isMacroID()) {
        auto macroName = static_cast<std::string>(Lexer::getImmediateMacroName(declStart, sm(), lo()));
        if (clazy::startsWithAny(macroName, { "Q_IMPORT_PLUGIN", "Q_CONSTRUCTOR_FUNCTION", "Q_DESTRUCTOR_FUNCTION"})) // Don't warn on these
            return;
    }

    CXXConstructExpr *ctorExpr = dyn_cast<CXXConstructExpr>(stm);
    if (!ctorExpr)
        return;

    auto ctorDecl = ctorExpr->getConstructor();
    auto recordDecl = ctorDecl ? ctorDecl->getParent() : nullptr;
    if (!recordDecl)
        return;

    if (recordDecl->hasTrivialDestructor()) {
        // Has a trivial dtor, but now lets check the ctors.

        if (ctorDecl->isDefaultConstructor() && recordDecl->hasTrivialDefaultConstructor()) {
            // both dtor and called ctor are trivial, no warning
            return;
        } else if (ctorDecl->isConstexpr()) {
            // Used ctor is constexpr, it's fine
            return;
        }
    }

    if (m_context->isQtDeveloper() && clazy::isBootstrapping(m_context->ci.getPreprocessorOpts()))
        return;

    StringRef className = clazy::name(recordDecl);
    if (!shouldIgnoreType(className)) {
        std::string error = string("non-POD static (") + className.data() + string(")");
        emitWarning(declStart, error);
    }

}
