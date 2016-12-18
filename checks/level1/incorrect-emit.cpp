/*
  This file is part of the clazy static checker.

  Copyright (C) 2016 Sergio Martins <smartins@kde.org>

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

#include "AccessSpecifierManager.h"
#include "incorrect-emit.h"
#include "Utils.h"
#include "HierarchyUtils.h"
#include "QtUtils.h"
#include "TypeUtils.h"
#include "checkmanager.h"

#include <clang/AST/AST.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/ExprCXX.h>
#include <clang/Parse/Parser.h>
#include <clang/Lex/Lexer.h>

using namespace clang;
using namespace std;

class QEmitPreprocessorCallbacks : public clang::PPCallbacks
{
    QEmitPreprocessorCallbacks(const QEmitPreprocessorCallbacks &) = delete;
public:
    QEmitPreprocessorCallbacks()
        : clang::PPCallbacks()
    {
        emitLocations.reserve(30); // bootstrap it
    }

    void MacroExpands(const Token &MacroNameTok, const MacroDefinition &,
                      SourceRange range, const MacroArgs *) override
    {
        IdentifierInfo *ii = MacroNameTok.getIdentifierInfo();
        if (!ii)
            return;

        if (ii->getName() == "emit" || ii->getName() == "Q_EMIT")
            emitLocations.push_back(range.getBegin());
    }

    vector<SourceLocation> emitLocations;
};

IncorrectEmit::IncorrectEmit(const std::string &name, const clang::CompilerInstance &ci)
    : CheckBase(name, ci)
    , m_preprocessorCallbacks(new QEmitPreprocessorCallbacks())
{
    CheckManager::instance()->enableAccessSpecifierManager(ci);
    Preprocessor &pi = m_ci.getPreprocessor();
    pi.addPPCallbacks(std::unique_ptr<PPCallbacks>(m_preprocessorCallbacks));
}


void IncorrectEmit::VisitStmt(Stmt *stmt)
{
    auto methodCall = dyn_cast<CXXMemberCallExpr>(stmt);
    if (!methodCall || !methodCall->getCalleeDecl())
        return;

    AccessSpecifierManager *accessSpecifierManager = CheckManager::instance()->accessSpecifierManager();

    auto method = dyn_cast<CXXMethodDecl>(methodCall->getCalleeDecl());
    if (!method)
        return;

    if (Stmt *parent = HierarchyUtils::parent(m_parentMap, methodCall)) {
        // Check if we're inside a chained call, such as: emit d_func()->mySignal()
        if (HierarchyUtils::getFirstParentOfType<CXXMemberCallExpr>(m_parentMap, parent))
            return;
    }

    const string filename = Utils::filenameForLoc(stmt->getLocStart(), sm());
    if (clazy_std::startsWith(filename, "moc_")) // blacklist
        return;

    const QtAccessSpecifierType type = accessSpecifierManager->qtAccessSpecifierType(method);
    if (type == QtAccessSpecifier_Unknown) {
        llvm::errs() << "error, couldn't find access specifier type\n";
        return;
    }

    const bool hasEmit = hasEmitKeyboard(methodCall);
    const string methodName = method->getQualifiedNameAsString();
    const bool isSignal = type == QtAccessSpecifier_Signal;
    if (isSignal && !hasEmit) {
        emitWarning(stmt, "Missing emit keyword on signal call " + methodName + "; " + filename);
    } else if (!isSignal && hasEmit) {
        emitWarning(stmt, "Emit keyword being used with non-signal " + methodName);
    }
}

bool IncorrectEmit::hasEmitKeyboard(CXXMemberCallExpr *call) const
{
    SourceLocation callLoc = call->getLocStart();
    if (callLoc.isMacroID())
        callLoc = sm().getFileLoc(callLoc);

    for (const SourceLocation &emitLoc : m_preprocessorCallbacks->emitLocations) {
        // TODO: Refactor Lexer stuff into a reusable method
        std::pair<FileID, unsigned> LocInfo = sm().getDecomposedLoc(emitLoc);
        bool InvalidTemp = false;
        StringRef File = sm().getBufferData(LocInfo.first, &InvalidTemp);
        if (InvalidTemp) {
            llvm::errs() << "Failed to get buffer data\n";
            return false;
        }
        const char *TokenBegin = File.data() + LocInfo.second;
        Lexer lexer(sm().getLocForStartOfFile(LocInfo.first), lo(), File.begin(),
                    TokenBegin, File.end());

        Token Tok;
        lexer.LexFromRawLexer(Tok);

        SourceLocation TokenLoc = Tok.getLocation();

        // Calculate how much whitespace needs to be skipped if any.
        unsigned NumWhitespaceChars = 0;
        const char *TokenEnd = sm().getCharacterData(TokenLoc) +
                Tok.getLength();
        unsigned char C = *TokenEnd;
        while (isHorizontalWhitespace(C)) {
            C = *(++TokenEnd);
            NumWhitespaceChars++;
        }

        // Skip \r, \n, \r\n, or \n\r
        if (C == '\n' || C == '\r') {
            char PrevC = C;
            C = *(++TokenEnd);
            NumWhitespaceChars++;
            if ((C == '\n' || C == '\r') && C != PrevC)
                NumWhitespaceChars++;
        }

        const SourceLocation nextTokenLoc = emitLoc.getLocWithOffset(Tok.getLength() + NumWhitespaceChars);
        if (nextTokenLoc == callLoc)
            return true;
    }

    return false;
}

REGISTER_CHECK_WITH_FLAGS("incorrect-emit", IncorrectEmit, CheckLevel1)
