/*
    SPDX-FileCopyrightText: 2019 Klarälvdalens Datakonsult AB a KDAB Group company info@kdab.com
    SPDX-FileContributor: Sérgio Martins <sergio.martins@kdab.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "MiniAstDumper.h"

#include <clang/Basic/FileManager.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendPluginRegistry.h>

using namespace clang;

MiniAstDumperASTAction::MiniAstDumperASTAction() = default;

bool MiniAstDumperASTAction::ParseArgs(const CompilerInstance &, const std::vector<std::string> &)
{
    return true;
}

std::unique_ptr<ASTConsumer> MiniAstDumperASTAction::CreateASTConsumer(CompilerInstance &, llvm::StringRef)
{
    return std::make_unique<MiniASTDumperConsumer>();
}

MiniASTDumperConsumer::MiniASTDumperConsumer() = default;

MiniASTDumperConsumer::~MiniASTDumperConsumer() = default;

bool MiniASTDumperConsumer::VisitDecl(Decl *decl)
{
    if (auto *rec = dyn_cast<CXXRecordDecl>(decl)) {
        llvm::errs() << "Found record: " << rec->getQualifiedNameAsString() << "\n";
    }

    return true;
}

bool MiniASTDumperConsumer::VisitStmt(Stmt *)
{
    return true;
}

void MiniASTDumperConsumer::HandleTranslationUnit(ASTContext &ctx)
{
    auto &sm = ctx.getSourceManager();
    const auto fileEntry = sm.getFileEntryRefForID(sm.getMainFileID());
    llvm::errs() << "Found TU: " << fileEntry->getName() << "\n";
    TraverseDecl(ctx.getTranslationUnitDecl());
}

static FrontendPluginRegistry::Add<MiniAstDumperASTAction> X2("clazyMiniAstDumper", "Clazy Mini AST Dumper plugin");
