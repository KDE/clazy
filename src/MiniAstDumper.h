/*
    SPDX-FileCopyrightText: 2019 Klarälvdalens Datakonsult AB a KDAB Group company info@kdab.com
    SPDX-FileContributor: Sérgio Martins <sergio.martins@kdab.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLAZY_MINI_AST_DUMPER
#define CLAZY_MINI_AST_DUMPER

#include <clang/AST/ASTConsumer.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Frontend/FrontendAction.h>
#include <llvm/ADT/StringRef.h>

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace clang
{
class CompilerInstance;
class ASTContext;
class Decl;
class Stmt;
}

class MiniAstDumperASTAction : public clang::PluginASTAction
{
public:
    MiniAstDumperASTAction();

protected:
    bool ParseArgs(const clang::CompilerInstance &ci, const std::vector<std::string> &args_) override;
    std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(clang::CompilerInstance &ci, llvm::StringRef) override;
};

class MiniASTDumperConsumer : public clang::ASTConsumer, public clang::RecursiveASTVisitor<MiniASTDumperConsumer>
{
public:
    explicit MiniASTDumperConsumer();
    ~MiniASTDumperConsumer() override;

    bool VisitDecl(clang::Decl *decl);
    bool VisitStmt(clang::Stmt *stm);
    void HandleTranslationUnit(clang::ASTContext &ctx) override;

private:
    MiniASTDumperConsumer(const MiniASTDumperConsumer &) = delete;
};

#endif
