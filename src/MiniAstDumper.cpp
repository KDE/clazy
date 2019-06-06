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

#include <fstream>

using namespace clang;
using namespace std;

MiniAstDumperASTAction::MiniAstDumperASTAction()
{
}

bool MiniAstDumperASTAction::ParseArgs(const CompilerInstance &, const std::vector<string> &)
{
    return true;
}

std::unique_ptr<ASTConsumer> MiniAstDumperASTAction::CreateASTConsumer(CompilerInstance &ci, llvm::StringRef)
{
    return std::unique_ptr<MiniASTDumperConsumer>(new MiniASTDumperConsumer(ci));
}

MiniASTDumperConsumer::MiniASTDumperConsumer(CompilerInstance &ci)
    : m_ci(ci)
{
    auto &sm = m_ci.getASTContext().getSourceManager();
    const FileEntry *fileEntry = sm.getFileEntryForID(sm.getMainFileID());

    llvm::errs() << "Found TU: " << fileEntry->getName() << "\n";

    const std::string currentCppFile = fileEntry->getName();


    cbor_encoder_init(&m_cborEncoder, m_cborBuf, sizeof(m_cborBuf), 0);
    cbor_encoder_create_map(&m_cborEncoder, &m_cborRootMapEncoder, 2);
    cbor_encode_text_stringz(&m_cborRootMapEncoder, "tu");
    cbor_encode_text_stringz(&m_cborRootMapEncoder, currentCppFile.c_str());
    cbor_encode_text_stringz(&m_cborRootMapEncoder, "stuff");
    cbor_encoder_create_array(&m_cborRootMapEncoder, &m_cborStuffArray, CborIndefiniteLength);


    //cbor_encode_boolean(&m_cborRootMapEncoder, some_value);
    //
}

MiniASTDumperConsumer::~MiniASTDumperConsumer()
{
    cbor_encoder_close_container(&m_cborRootMapEncoder, &m_cborRootMapEncoder);
    cbor_encoder_close_container(&m_cborEncoder, &m_cborStuffArray);

    size_t size = cbor_encoder_get_buffer_size(&m_cborEncoder, m_cborBuf);

    std::ofstream myFile ("data.bin", std::ios::out | ios::binary);
    myFile.write(reinterpret_cast<char*>(m_cborBuf), long(size));
}

bool MiniASTDumperConsumer::VisitDecl(Decl *decl)
{
    if (auto tsd = dyn_cast<ClassTemplateSpecializationDecl>(decl)) {
        llvm::errs() << "ClassTemplateSpecializationDecl: "  + tsd->getQualifiedNameAsString() + "\n";
    } else if (auto rec = dyn_cast<CXXRecordDecl>(decl)) {
        if (!rec->isThisDeclarationADefinition()) {
            // No forward-declarations
            return true;
        }

        if (rec->getDescribedClassTemplate()) {
            // This is a template. We'll rather print it's specializations when catching ClassTemplateDecl
            return true;
        }
        llvm::errs() << "Found record: " << rec->getQualifiedNameAsString()
                     << "; this=" << rec
                     << "\n";
    } else if (auto ctd = dyn_cast<ClassTemplateDecl>(decl)) {
        llvm::errs() << "Found template: " << ctd->getNameAsString()
                     << "; this=" << ctd
                     << "\n";
        for (auto s : ctd->specializations()) {
            llvm::errs() << "Found specialization: " << s->getQualifiedNameAsString() << "\n";
            auto &args = s->getTemplateArgs();
            const unsigned int count = args.size();
            for (unsigned int i = 0; i < count; ++i) {
                args.get(i).print(PrintingPolicy({}), llvm::errs());
                llvm::errs() << "\n";
            }
        }
    }

    return true;
}

bool MiniASTDumperConsumer::VisitStmt(Stmt *)
{
    return true;
}

void MiniASTDumperConsumer::HandleTranslationUnit(ASTContext &ctx)
{
    TraverseDecl(ctx.getTranslationUnitDecl());
}

static FrontendPluginRegistry::Add<MiniAstDumperASTAction>
X2("clazyMiniAstDumper", "Clazy Mini AST Dumper plugin");
