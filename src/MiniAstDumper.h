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

#ifndef CLAZY_MINI_AST_DUMPER
#define CLAZY_MINI_AST_DUMPER

#include <clang/AST/ASTConsumer.h>
#include <clang/Frontend/FrontendAction.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <llvm/ADT/StringRef.h>

#include <memory>
#include <vector>
#include <string>
#include <utility>

namespace clang {
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

class MiniASTDumperConsumer
    : public clang::ASTConsumer
    , public clang::RecursiveASTVisitor<MiniASTDumperConsumer>
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
