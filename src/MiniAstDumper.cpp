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
#include "QtUtils.h"
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendPluginRegistry.h>

#include <fstream>
#include <unistd.h>
#include <limits.h>

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
    m_currentCppFile = fileEntry->getName();

    m_cborBuf = reinterpret_cast<uint8_t*>(malloc(m_bufferSize));

    cbor_encoder_init(&m_cborEncoder, m_cborBuf, m_bufferSize, 0);
    cborCreateMap(&m_cborEncoder, &m_cborRootMapEncoder, 4);
    cborEncodeString(m_cborRootMapEncoder, "tu");
    cborEncodeString(m_cborRootMapEncoder, m_currentCppFile.c_str());

    char cwd[4096]; // TODO: Just use std::filesystem::current_path
    cborEncodeString(m_cborRootMapEncoder, "cwd");
    cborEncodeString(m_cborRootMapEncoder, std::string(getcwd(cwd, sizeof(cwd))).c_str());

    cborEncodeString(m_cborRootMapEncoder, "stuff");
    cborCreateArray(&m_cborRootMapEncoder, &m_cborStuffArray, CborIndefiniteLength);
}

MiniASTDumperConsumer::~MiniASTDumperConsumer()
{
    cborCloseContainer(&m_cborRootMapEncoder, &m_cborStuffArray);

    cborEncodeString(m_cborRootMapEncoder, "files");
    dumpFileMap(&m_cborRootMapEncoder);

    cborCloseContainer(&m_cborEncoder, &m_cborRootMapEncoder);


    size_t size = cbor_encoder_get_buffer_size(&m_cborEncoder, m_cborBuf);

    const std::string cborFileName = m_currentCppFile + ".cbor";

    std::ofstream myFile(cborFileName, std::ios::out | ios::binary);
    myFile.write(reinterpret_cast<char*>(m_cborBuf), long(size));
    delete m_cborBuf;
}

bool MiniASTDumperConsumer::VisitDecl(Decl *decl)
{
    if (auto tsd = dyn_cast<ClassTemplateSpecializationDecl>(decl)) {
        //llvm::errs() << "ClassTemplateSpecializationDecl: "  + tsd->getQualifiedNameAsString() + "\n";
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
        /*llvm::errs() << "Found template: " << ctd->getNameAsString()
                     << "; this=" << ctd
                     << "\n";*/
        for (auto s : ctd->specializations()) {
           /* llvm::errs() << "Found specialization: " << s->getQualifiedNameAsString() << "\n";
            auto &args = s->getTemplateArgs();
            const unsigned int count = args.size();
            for (unsigned int i = 0; i < count; ++i) {
                args.get(i).print(PrintingPolicy({}), llvm::errs());
                llvm::errs() << "\n";
            }*/
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
    cborCreateMap(encoder, &recordMap, 2);

    cborEncodeString(recordMap, "name");
    cborEncodeString(recordMap, method->getQualifiedNameAsString().c_str());

    cborEncodeString(recordMap, "id");
    cborEncodeInt(recordMap, int64_t(method));

    cborCloseContainer(encoder, &recordMap);
}

void MiniASTDumperConsumer::dumpCXXRecordDecl(CXXRecordDecl *rec, CborEncoder *encoder)
{
    CborEncoder recordMap;
    cborCreateMap(encoder, &recordMap, CborIndefiniteLength);

    cborEncodeString(recordMap, "type");
    cborEncodeInt(recordMap, rec->getDeclKind());

    cborEncodeString(recordMap, "name");
    cborEncodeString(recordMap, rec->getQualifiedNameAsString().c_str());

    cborEncodeString(recordMap, "loc");
    dumpLocation(clazy::getLocStart(rec), &recordMap);

    if (clazy::isQObject(rec)) { // TODO: Use flags
        cborEncodeString(recordMap, "isQObject");
        cborEncodeBool(recordMap, true);
    }

    cborEncodeString(recordMap, "methods");
    CborEncoder cborMethodList;
    cborCreateArray(&recordMap, &cborMethodList, CborIndefiniteLength);
    for (auto method : rec->methods()) {
        dumpCXXMethodDecl(method, &cborMethodList);
    }
    cborCloseContainer(&recordMap, &cborMethodList);
    cborCloseContainer(&m_cborStuffArray, &recordMap);
}

void MiniASTDumperConsumer::dumpCallExpr(CallExpr *callExpr, CborEncoder *encoder)
{
    FunctionDecl *func = callExpr->getDirectCallee();
    if (!func || !func->getDeclName().isIdentifier())
        return;

    CborEncoder callMap;
    cborCreateMap(encoder, &callMap, 3);

    cborEncodeString(callMap, "type");
    cborEncodeInt(callMap, callExpr->getStmtClass());

    cborEncodeString(callMap, "calleeName"); // TODO: replace with ID
    cborEncodeString(callMap, func->getQualifiedNameAsString().c_str());

    cborEncodeString(callMap, "loc");
    dumpLocation(clazy::getLocStart(callExpr), &callMap);

    cborCloseContainer(encoder, &callMap);
}

void MiniASTDumperConsumer::dumpLocation(SourceLocation loc, CborEncoder *encoder)
{
    CborEncoder locMap;
    cborCreateMap(encoder, &locMap, 3);

    auto &sm = m_ci.getSourceManager();
    const FileID fileId = sm.getFileID(loc);

    m_fileIds[fileId.getHashValue()] = sm.getFilename(loc).str();
    cborEncodeString(locMap, "fileId");
    cborEncodeInt(locMap, fileId.getHashValue());

    auto ploc = sm.getPresumedLoc(loc);
    cborEncodeString(locMap, "line");

    cborEncodeInt(locMap,  ploc.getLine());
    cborEncodeString(locMap, "column");
    cborEncodeInt(locMap,  ploc.getColumn());

    cborCloseContainer(encoder, &locMap);
}

void MiniASTDumperConsumer::dumpFileMap(CborEncoder *encoder)
{
    CborEncoder fileMap;
    cborCreateMap(encoder, &fileMap, m_fileIds.size());

    for (auto it : m_fileIds) {
        cborEncodeString(fileMap, std::to_string(it.first).c_str());
        cborEncodeString(fileMap, it.second.c_str());
    }

    cborCloseContainer(encoder, &fileMap);
}

void MiniASTDumperConsumer::cborEncodeString(CborEncoder &enc, const char *str)
{
    if (cbor_encode_text_stringz(&enc, str) != CborNoError)
        llvm::errs() << "cborEncodeString error\n";
}

void MiniASTDumperConsumer::cborEncodeInt(CborEncoder &enc, int64_t v)
{
    if (cbor_encode_int(&enc, v) != CborNoError)
        llvm::errs() << "cborEncodeInt error\n";
}

void MiniASTDumperConsumer::cborEncodeBool(CborEncoder &enc, bool b)
{
    if (cbor_encode_boolean(&enc, b) != CborNoError)
        llvm::errs() << "cborEncodeBool error\n";
}

void MiniASTDumperConsumer::cborCreateMap(CborEncoder *encoder, CborEncoder *mapEncoder, size_t length)
{
    if (cbor_encoder_create_map(encoder, mapEncoder, length) != CborNoError)
        llvm::errs() << "cborCreateMap error\n";
}

void MiniASTDumperConsumer::cborCreateArray(CborEncoder *encoder, CborEncoder *arrayEncoder, size_t length)
{
    if (cbor_encoder_create_array(encoder, arrayEncoder, length) != CborNoError)
        llvm::errs() << "cborCreateMap error\n";
}

void MiniASTDumperConsumer::cborCloseContainer(CborEncoder *encoder, const CborEncoder *containerEncoder)
{
    if (cbor_encoder_close_container(encoder, containerEncoder) != CborNoError)
        llvm::errs() << "cborCloseContainer error\n";
}

static FrontendPluginRegistry::Add<MiniAstDumperASTAction>
X2("clazyMiniAstDumper", "Clazy Mini AST Dumper plugin");
