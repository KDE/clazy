/*
    SPDX-FileCopyrightText: 2020 The Qt Company Ltd.
    SPDX-FileCopyrightText: 2020 Lucie Gerard <lucie.gerard@qt.io>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "qt6-fwd-fixes.h"
#include "ClazyContext.h"
#include "FixItUtils.h"
#include "StringUtils.h"
#include "Utils.h"

#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/Type.h>
#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/LLVM.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Lex/Lexer.h>
#include <llvm/ADT/ArrayRef.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Casting.h>

#include <clang/Basic/Specifiers.h>

using namespace clang;

Qt6FwdFixes::Qt6FwdFixes(const std::string &name, ClazyContext *context)
    : CheckBase(name, context, Option_CanIgnoreIncludes)
{
    enablePreProcessorCallbacks();
    context->enablePreprocessorVisitor();
}

static std::set<std::string> interestingFwdDecl = {
    "QCache", "QHash",   "QMap",        "QMultiHash",     "QMultiMap", "QPair",    "QQueue",       "QSet",        "QStack",       "QVarLengthArray",
    "QList",  "QVector", "QStringList", "QByteArrayList", "QMetaType", "QVariant", "QVariantList", "QVariantMap", "QVariantHash", "QVariantPair"};

SourceLocation locForNextSemiColon(SourceLocation loc, const clang::SourceManager &sm, const clang::LangOptions &lo)
{
    std::pair<FileID, unsigned> locInfo = sm.getDecomposedLoc(loc);
    bool InvalidTemp = false;
    StringRef File = sm.getBufferData(locInfo.first, &InvalidTemp);
    if (InvalidTemp) {
        return {};
    }

    const char *TokenBegin = File.data() + locInfo.second;
    Lexer lexer(sm.getLocForStartOfFile(locInfo.first), lo, File.begin(), TokenBegin, File.end());

    Token Tok;
    lexer.LexFromRawLexer(Tok);

    SourceLocation TokenLoc = Tok.getLocation();

    // Calculate how much chars needs to be skipped until the ';'
    // plus white spaces and \n or \r  after
    unsigned NumCharsUntilSemiColon = 0;
    unsigned NumWhitespaceChars = 0;
    const char *TokenEnd = sm.getCharacterData(TokenLoc) + Tok.getLength();
    unsigned char C = *TokenEnd;

    while (C != ';') {
        C = *(++TokenEnd);
        NumCharsUntilSemiColon++;
    }
    C = *(++TokenEnd);
    while (isHorizontalWhitespace(C)) {
        C = *(++TokenEnd);
        NumWhitespaceChars++;
    }
    // Skip \r, \n, \r\n, or \n\r
    if (C == '\n' || C == '\r') {
        char PrevC = C;
        C = *(++TokenEnd);
        NumWhitespaceChars++;
        if ((C == '\n' || C == '\r') && C != PrevC) {
            NumWhitespaceChars++;
        }
    }
    return loc.getLocWithOffset(Tok.getLength() + NumCharsUntilSemiColon + NumWhitespaceChars + 1);
}

void Qt6FwdFixes::VisitDecl(clang::Decl *decl)
{
    auto *recDecl = dyn_cast<CXXRecordDecl>(decl);
    if (!recDecl) {
        return;
    }
    auto *parent = recDecl->getParent();
    std::string parentType = parent->getDeclKindName();
    if (parentType != "TranslationUnit") {
        return;
    }
    if (recDecl->hasDefinition()) {
        return;
    }
    if (interestingFwdDecl.find(recDecl->getNameAsString()) == interestingFwdDecl.end()) {
        return;
    }

    const std::string currentFile = sm().getFilename(decl->getLocation()).str();
    if (m_currentFile != currentFile) {
        m_currentFile = currentFile;
        m_including_qcontainerfwd = false;
        if (m_qcontainerfwd_included_in_files.find(currentFile) != m_qcontainerfwd_included_in_files.end()) {
            m_including_qcontainerfwd = true;
        }
    }

    SourceLocation endLoc = locForNextSemiColon(recDecl->getBeginLoc(), sm(), lo());

    SourceLocation beginLoc;
    auto *tempclass = recDecl->getDescribedClassTemplate();
    if (tempclass) {
        beginLoc = tempclass->getBeginLoc();
    } else {
        beginLoc = recDecl->getBeginLoc();
    }

    std::vector<FixItHint> fixits;
    std::string message;
    auto warningLocation = beginLoc;
    SourceRange fixitRange = SourceRange(beginLoc, endLoc);

    CharSourceRange controledFixitRange = CharSourceRange(fixitRange, false);
    if (!m_including_qcontainerfwd) {
        fixits.push_back(FixItHint::CreateReplacement(controledFixitRange, "#include <QtCore/qcontainerfwd.h>\n"));
    } else {
        fixits.push_back(FixItHint::CreateRemoval(controledFixitRange));
    }

    message += "Using forward declaration of ";
    message += recDecl->getNameAsString();
    message += ".";
    if (m_including_qcontainerfwd) {
        message += " (already)";
    }
    message += " Including <QtCore/qcontainerfwd.h> instead.";

    emitWarning(warningLocation, message, fixits);
    m_including_qcontainerfwd = true;
    return;
}

void Qt6FwdFixes::VisitInclusionDirective(clang::SourceLocation HashLoc,
                                          const clang::Token & /*IncludeTok*/,
                                          clang::StringRef FileName,
                                          bool /*IsAngled*/,
                                          clang::CharSourceRange /*FilenameRange*/,
                                          clazy::OptionalFileEntryRef /*File*/,
                                          clang::StringRef /*SearchPath*/,
                                          clang::StringRef /*RelativePath*/,
                                          const clang::Module * /*SuggestedModule*/,
                                          bool /*ModuleImported*/,
                                          clang::SrcMgr::CharacteristicKind /*FileType*/)
{
    auto current_file = sm().getFilename(HashLoc);
    if (FileName.str() == "QtCore/qcontainerfwd.h") {
        m_qcontainerfwd_included_in_files.insert(current_file);
        return;
    }
}
