/*
    SPDX-FileCopyrightText: 2015 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "missing-qobject-macro.h"
#include "ClazyContext.h"
#include "FixItUtils.h"
#include "PreProcessorVisitor.h"
#include "QtUtils.h"

#include <clang/AST/DeclBase.h>
#include <clang/AST/DeclCXX.h>
#include <clang/Basic/IdentifierTable.h>
#include <clang/Basic/LLVM.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Lex/Token.h>
#include <filesystem>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Casting.h>

using namespace clang;

MissingQObjectMacro::MissingQObjectMacro(const std::string &name, ClazyContext *context)
    : CheckBase(name, context)
{
    context->enablePreprocessorVisitor();
}

void MissingQObjectMacro::VisitMacroExpands(const clang::Token &MacroNameTok, const clang::SourceRange &range, const MacroInfo *)
{
    IdentifierInfo *ii = MacroNameTok.getIdentifierInfo();
    if (ii && ii->getName() == "Q_OBJECT") {
        registerQ_OBJECT(range.getBegin());
    }
}

void MissingQObjectMacro::VisitDecl(clang::Decl *decl)
{
    auto *record = dyn_cast<CXXRecordDecl>(decl);
    if (!record || !record->hasDefinition() || record->getDefinition() != record || !clazy::isQObject(record)) {
        return;
    }

    if (record->getDescribedClassTemplate() != nullptr) { // moc doesn't accept Q_OBJECT in templates
        return;
    }

    if (isa<CXXRecordDecl>(record->getDeclContext())) { // moc doesn't accept Q_OBJECT in nested classes
        return;
    }

    if (m_context->usingPreCompiledHeaders()) {
        return;
    }

    const SourceLocation startLoc = decl->getBeginLoc();

    for (const SourceLocation &loc : m_qobjectMacroLocations) {
        if (sm().getFileID(loc) != sm().getFileID(startLoc)) {
            continue; // Different file
        }

        if (sm().isBeforeInSLocAddrSpace(startLoc, loc) && sm().isBeforeInSLocAddrSpace(loc, decl->getEndLoc())) {
            return; // We found a Q_OBJECT after start and before end, it's ours.
        }
    }

    std::vector<FixItHint> fixits;
    const SourceLocation pos = record->getBraceRange().getBegin().getLocWithOffset(1);
    fixits.push_back(clazy::createInsertion(pos, "\n\tQ_OBJECT"));

    const std::string fileName = static_cast<std::string>(sm().getFilename(startLoc));
    if (clazy::endsWith(fileName, ".cpp")) {
        const std::string basename = std::filesystem::path(fileName).stem().string();

        if (!m_hasAddedMocFile && !m_context->preprocessorVisitor->hasInclude(basename + ".moc", false)) {
            const SourceLocation pos = sm().getLocForEndOfFile(sm().getFileID(startLoc));
            fixits.push_back(clazy::createInsertion(pos, "\n#include \"" + basename + ".moc\"\n"));
            m_hasAddedMocFile = true;
        }
    }

    emitWarning(startLoc, record->getQualifiedNameAsString() + " is missing a Q_OBJECT macro", fixits);
}

void MissingQObjectMacro::registerQ_OBJECT(SourceLocation loc)
{
    m_qobjectMacroLocations.push_back(loc);
}
