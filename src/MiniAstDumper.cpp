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
#include "AccessSpecifierManager.h"
#include "clazy_stl.h"
#include "FunctionUtils.h"
#include "QtUtils.h"
#include "StringUtils.h"
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendPluginRegistry.h>

#include <fstream>
#include <unistd.h>
#include <limits.h>

using namespace clang;
using namespace std;

enum ClassFlag {
    ClassFlag_None = 0,
    ClassFlag_QObject = 1
};

enum MethodFlag {
    MethodFlag_None = 0,
    MethodFlag_Signal = 1
};

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
    , m_accessSpecifierManager(new AccessSpecifierManager(ci))
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
    m_accessSpecifierManager->VisitDeclaration(decl);

    auto &sm = m_ci.getSourceManager();
    const SourceLocation loc = clazy::getLocStart(decl);

    if (sm.isInSystemHeader(loc)) {
        // We're not interested in those
        return true;
    }

    if (auto tsd = dyn_cast<ClassTemplateSpecializationDecl>(decl)) {
        // llvm::errs() << "ClassTemplateSpecializationDecl: "  + tsd->getQualifiedNameAsString() + "\n";
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
                     << "\n";
        for (auto s : ctd->specializations()) {
            llvm::errs() << "Found specialization: " << s->getQualifiedNameAsString() << "\n";
            auto &args = s->getTemplateArgs();
            const unsigned int count = args.size();
            for (unsigned int i = 0; i < count; ++i) {
                args.get(i).print(PrintingPolicy({}), llvm::errs());
                llvm::errs() << "\n";
            }
        } */
    } else if (auto func = dyn_cast<FunctionDecl>(decl)) {
        if (isa<CXXMethodDecl>(decl)) // Methods are handled when processing the class
            return true;

        if (func->getTemplatedKind() == FunctionDecl::TK_FunctionTemplate) {
            // Already handled when catching FunctionTemplateDecl. When we write func->getTemplatedDecl().

            if (func->getQualifiedNameAsString() == "qobject_cast") {
                llvm::errs() << "skipping! " << func << "\n";
            }

            return true;
        }

        dumpFunctionDecl(func, &m_cborStuffArray);
    } else if (auto func = dyn_cast<FunctionTemplateDecl>(decl)) {

        dumpFunctionDecl(func->getTemplatedDecl(), &m_cborStuffArray);

        for (auto s : func->specializations()) {
            dumpFunctionDecl(s, &m_cborStuffArray);
        }
    }

    return true;
}

bool MiniASTDumperConsumer::VisitStmt(Stmt *stmt)
{
    auto &sm = m_ci.getSourceManager();
    const SourceLocation loc = clazy::getLocStart(stmt);

    if (sm.isInSystemHeader(loc)) {
        // We're not interested in those
        return true;
    }

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
    CborEncoder methodMap;
    cborCreateMap(encoder, &methodMap, CborIndefiniteLength);

    cborEncodeString(methodMap, "name");
    cborEncodeString(methodMap, method->getQualifiedNameAsString().c_str());

    cborEncodeString(methodMap, "id");
    cborEncodeInt(methodMap, int64_t(method));

    int64_t flags = 0;

    if (m_accessSpecifierManager->qtAccessSpecifierType(method) == QtAccessSpecifier_Signal)
        flags |= MethodFlag_Signal;

    if (flags) {
        cborEncodeString(methodMap, "method_flags");
        cborEncodeInt(methodMap, flags);
    }

    cborEncodeString(methodMap, "overrides");
    CborEncoder cborOverriddenMethodsList;
    cborCreateArray(&methodMap, &cborOverriddenMethodsList, CborIndefiniteLength);
    for (auto om : method->overridden_methods()) {
        cborEncodeInt(cborOverriddenMethodsList, int64_t(om));
    }
    cborCloseContainer(&methodMap, &cborOverriddenMethodsList);

    cborCloseContainer(encoder, &methodMap);
}

void MiniASTDumperConsumer::dumpFunctionDecl(FunctionDecl *func, CborEncoder *encoder)
{

    if (func->getQualifiedNameAsString() == "qobject_cast") {
        llvm::errs() << "registering! " << func
                     << " " << func->getBeginLoc().printToString(m_ci.getSourceManager())
                     << "\n";
    }

    CborEncoder recordMap;
    cborCreateMap(encoder, &recordMap, 4);

    cborEncodeString(recordMap, "name");
    cborEncodeString(recordMap, func->getQualifiedNameAsString().c_str());

    cborEncodeString(recordMap, "type");
    cborEncodeInt(recordMap, func->FunctionDecl::getDeclKind());

    cborEncodeString(recordMap, "id");
    cborEncodeInt(recordMap, int64_t(func));

    cborEncodeString(recordMap, "loc");
    const SourceLocation loc = clazy::getLocStart(func);
    dumpLocation(loc, &recordMap);

    cborCloseContainer(encoder, &recordMap);
}

void MiniASTDumperConsumer::dumpCXXRecordDecl(CXXRecordDecl *rec, CborEncoder *encoder)
{
    if (rec->isUnion())
        return;

    CborEncoder recordMap;
    cborCreateMap(encoder, &recordMap, CborIndefiniteLength);

    cborEncodeString(recordMap, "type");
    cborEncodeInt(recordMap, rec->getDeclKind());

    cborEncodeString(recordMap, "name");
    cborEncodeString(recordMap, rec->getQualifiedNameAsString().c_str());

    cborEncodeString(recordMap, "loc");
    const SourceLocation loc = clazy::getLocStart(rec);
    dumpLocation(loc, &recordMap);

    int64_t flags = 0;

    if (clazy::isQObject(rec))
        flags |= ClassFlag_QObject;

    if (flags) {
        cborEncodeString(recordMap, "class_flags");
        cborEncodeInt(recordMap, flags);
    }

    cborEncodeString(recordMap, "methods");
    CborEncoder cborMethodList;
    cborCreateArray(&recordMap, &cborMethodList, CborIndefiniteLength);
    for (auto method : rec->methods()) {
        dumpCXXMethodDecl(method, &cborMethodList);
    }

    cborCloseContainer(&recordMap, &cborMethodList);
    cborCloseContainer(&m_cborStuffArray, &recordMap);

    // Sanity:
    if (rec->getQualifiedNameAsString().empty()) {
        llvm::errs() << "Record has no name. loc=" << loc.printToString(m_ci.getSourceManager()) << "\n";
    }
}

void MiniASTDumperConsumer::dumpCallExpr(CallExpr *callExpr, CborEncoder *encoder)
{
    FunctionDecl *func = callExpr->getDirectCallee();
    if (!func || !func->getDeclName().isIdentifier())
        return;

    const bool isBuiltin = func->getBuiltinID() != 0;
    if (isBuiltin) //We don't need them now
        return;

    auto method = dyn_cast<CXXMethodDecl>(func);
    if (method) {
        // For methods we store the declaration
        func = clazy::getFunctionDeclaration(method);
    }

    if (func->getQualifiedNameAsString() == "qobject_cast") {
        llvm::errs() << "foo-call "
                     << func
                     << "; original=" << callExpr->getDirectCallee()
                     << "; funcloc " << func->getBeginLoc().printToString(m_ci.getSourceManager())
                     << "; original-loc= " << callExpr->getDirectCallee()->getBeginLoc().printToString(m_ci.getSourceManager())

                     << "; " << func->getTemplatedKind() << " ; " << callExpr->getDirectCallee()->getTemplatedKind()

                     << "\n";
    }


    CborEncoder callMap;
    cborCreateMap(encoder, &callMap, 3);

    cborEncodeString(callMap, "stmt_type");
    cborEncodeInt(callMap, callExpr->getStmtClass());

    cborEncodeString(callMap, "calleeId");
    cborEncodeInt(callMap, int64_t(func));

    cborEncodeString(callMap, "loc");
    dumpLocation(clazy::getLocStart(callExpr), &callMap);

    cborCloseContainer(encoder, &callMap);
}

void MiniASTDumperConsumer::dumpLocation(SourceLocation loc, CborEncoder *encoder)
{
    CborEncoder locMap;
    cborCreateMap(encoder, &locMap, CborIndefiniteLength);

    auto &sm = m_ci.getSourceManager();

    if (loc.isMacroID()) {
        /// The place where the macro is defined:
        SourceLocation spellingLoc = sm.getSpellingLoc(loc);
        const FileID fileId = sm.getFileID(spellingLoc);
        m_fileIds[fileId.getHashValue()] = sm.getFilename(spellingLoc).str();

        cborEncodeString(locMap, "spellingFileId");
        cborEncodeInt(locMap, fileId.getHashValue());

        cborEncodeString(locMap, "spellingLine");
        cborEncodeInt(locMap, sm.getSpellingLineNumber(loc));

        cborEncodeString(locMap, "spellingColumn");
        cborEncodeInt(locMap, sm.getSpellingColumnNumber(loc));

        /// Get the place where the macro is used:
        loc = sm.getExpansionLoc(loc);
    }

    const FileID fileId = sm.getFileID(loc);
    m_fileIds[fileId.getHashValue()] = sm.getFilename(loc).str();

    if (sm.getFilename(loc).empty()) {
        // Shouldn't happen
        llvm::errs() << "Invalid filename for " << loc.printToString(sm) <<  "\n";
    }

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
