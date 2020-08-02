/*
    This file is part of the clazy static checker.

    Copyright (C) 2019 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
    Author: Sérgio Martins <sergio.martins@kdab.com>

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

#include "MiniAstDumper.h"

#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendPluginRegistry.h>
#include <clang/Basic/FileManager.h>

using namespace clang;
using namespace std;

MiniAstDumperASTAction::MiniAstDumperASTAction()
{
}

bool MiniAstDumperASTAction::ParseArgs(const CompilerInstance &, const std::vector<string> &)
{
    return true;
}

std::unique_ptr<ASTConsumer> MiniAstDumperASTAction::CreateASTConsumer(CompilerInstance &, llvm::StringRef)
{
    return std::unique_ptr<MiniASTDumperConsumer>(new MiniASTDumperConsumer());
}

MiniASTDumperConsumer::MiniASTDumperConsumer()
{
}

MiniASTDumperConsumer::~MiniASTDumperConsumer()
{
}

bool MiniASTDumperConsumer::VisitDecl(Decl *decl)
{
    if (auto rec = dyn_cast<CXXRecordDecl>(decl)) {
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
    const FileEntry *fileEntry = sm.getFileEntryForID(sm.getMainFileID());
    llvm::errs() << "Found TU: " << fileEntry->getName() << "\n";
    TraverseDecl(ctx.getTranslationUnitDecl());
}

static FrontendPluginRegistry::Add<MiniAstDumperASTAction>
X2("clazyMiniAstDumper", "Clazy Mini AST Dumper plugin");
