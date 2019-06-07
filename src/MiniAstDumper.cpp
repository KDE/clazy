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
#include "SourceCompatibilityHelpers.h"
#include "clazy_stl.h"
#include "StringUtils.h"
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

    m_cborBuf = reinterpret_cast<uint8_t*>(malloc(m_bufferSize));

    const std::string currentCppFile = fileEntry->getName();

    cbor_encoder_init(&m_cborEncoder, m_cborBuf, m_bufferSize, 0);
    cbor_encoder_create_map(&m_cborEncoder, &m_cborRootMapEncoder, 2);
    cbor_encode_text_stringz(&m_cborRootMapEncoder, "tu");
    cbor_encode_text_stringz(&m_cborRootMapEncoder, currentCppFile.c_str());
    cbor_encode_text_stringz(&m_cborRootMapEncoder, "stuff");
    cbor_encoder_create_array(&m_cborRootMapEncoder, &m_cborStuffArray, CborIndefiniteLength);
}

MiniASTDumperConsumer::~MiniASTDumperConsumer()
{
    cbor_encoder_close_container(&m_cborRootMapEncoder, &m_cborRootMapEncoder);
    cbor_encoder_close_container(&m_cborEncoder, &m_cborStuffArray);

    size_t size = cbor_encoder_get_buffer_size(&m_cborEncoder, m_cborBuf);

    std::ofstream myFile ("data.bin", std::ios::out | ios::binary);
    myFile.write(reinterpret_cast<char*>(m_cborBuf), long(size));
    delete m_cborBuf;
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

        dumpCXXRecordDecl(rec, &m_cborStuffArray);
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

bool MiniASTDumperConsumer::VisitStmt(Stmt *stmt)
{
    if (auto callExpr = dyn_cast<CallExpr>(stmt)) {
        dumpCallExpr(callExpr, &m_cborStuffArray);
    }

    return true;
}

void MiniASTDumperConsumer::HandleTranslationUnit(ASTContext &ctx)
{
    TraverseDecl(ctx.getTranslationUnitDecl());
}

void MiniASTDumperConsumer::dumpCXXMethodDecl(CXXMethodDecl *method, CborEncoder *encoder)
{
    CborEncoder recordMap;
    cbor_encoder_create_map(encoder, &recordMap, 5);

    //cborEncodeString(recordMap, "type");
    //cborEncodeInt(recordMap, method->getDeclKind());

    cborEncodeString(recordMap, "name");
    cborEncodeString(recordMap, method->getQualifiedNameAsString().c_str());

    cborEncodeString(recordMap, "id");
    cborEncodeInt(recordMap, int64_t(method));

    //cborEncodeString(recordMap, "loc");
    //cborEncodeString(recordMap, clazy::getLocStart(method).printToString(m_ci.getSourceManager()).c_str());

    cbor_encoder_close_container(encoder, &recordMap);
}

void MiniASTDumperConsumer::dumpCXXRecordDecl(CXXRecordDecl *rec, CborEncoder *encoder)
{
    CborEncoder recordMap;
    cbor_encoder_create_map(encoder, &recordMap, 4);

    cborEncodeString(recordMap, "type");
    cborEncodeInt(recordMap, rec->getDeclKind());

    cborEncodeString(recordMap, "name");
    cborEncodeString(recordMap, rec->getQualifiedNameAsString().c_str());

    cborEncodeString(recordMap, "id");
    cborEncodeInt(recordMap, int64_t(rec));

    cborEncodeString(recordMap, "loc"); // TODO: replace with file id
    cborEncodeString(recordMap, clazy::getLocStart(rec).printToString(m_ci.getSourceManager()).c_str());

    cbor_encoder_close_container(&m_cborStuffArray, &recordMap);

    CborEncoder cborMethodList;
    cbor_encoder_create_array(encoder, &cborMethodList, CborIndefiniteLength);
    for (auto method : rec->methods()) {
        dumpCXXMethodDecl(method, &cborMethodList);
    }
    cbor_encoder_close_container(encoder, &cborMethodList);

}

void MiniASTDumperConsumer::dumpCallExpr(CallExpr *callExpr, CborEncoder *encoder)
{
    if (!callExpr->getDirectCallee())
        return;

    CborEncoder callMap;
    cbor_encoder_create_map(encoder, &callMap, 1);

    cborEncodeString(callMap, "calleeName"); // TODO: replace with ID
    cborEncodeString(callMap, clazy::name(callExpr->getDirectCallee()).str().c_str());

    cbor_encoder_close_container(encoder, &callMap);
}

void MiniASTDumperConsumer::cborEncodeString(CborEncoder &enc, const char *str)
{
    cbor_encode_text_stringz(&enc, str);
}

void MiniASTDumperConsumer::cborEncodeInt(CborEncoder &enc, int64_t v)
{
    cbor_encode_int(&enc, v);
}

static FrontendPluginRegistry::Add<MiniAstDumperASTAction>
X2("clazyMiniAstDumper", "Clazy Mini AST Dumper plugin");
