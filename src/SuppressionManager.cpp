/*
    This file is part of the clazy static checker.

    SPDX-FileCopyrightText: 2016 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "SuppressionManager.h"
#include "SourceCompatibilityHelpers.h"
#include "clazy_stl.h"

#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Basic/TokenKinds.h>
#include <clang/Lex/Token.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/raw_ostream.h>

#include <vector>

using namespace clang;

SuppressionManager::SuppressionManager()
{
}

bool SuppressionManager::isSuppressed(const std::string &checkName,
                                      clang::SourceLocation loc,
                                      const clang::SourceManager &sm,
                                      const clang::LangOptions &lo) const
{
    if (loc.isMacroID()) {
        loc = sm.getExpansionLoc(loc);
    }

    FileID fileID = sm.getFileID(loc);
    if (fileID.isInvalid()) {
        return false;
    }

    auto it = m_processedFileIDs.find(fileID.getHashValue());
    const bool notProcessedYet = (it == m_processedFileIDs.cend());
    if (notProcessedYet) {
        parseFile(fileID, sm, lo);
        it = m_processedFileIDs.find(fileID.getHashValue());
    }

    Suppressions &suppressions = (*it).second;

    // Case 1: clazy:skip, the whole file is skipped, regardless of which check
    if (suppressions.skipEntireFile) {
        return true;
    }

    // Case 2: clazy:excludeall=foo, the check foo will be ignored for this file
    const bool checkIsSuppressed = suppressions.checksToSkip.find(checkName) != suppressions.checksToSkip.cend();
    if (checkIsSuppressed) {
        return true;
    }

    // Case 3: clazy:exclude=foo, the check foo will be ignored for this file in this line number
    if (loc.isInvalid()) {
        return false;
    }

    const int lineNumber = sm.getSpellingLineNumber(loc);
    if (suppressions.skipNextLine.count(lineNumber) > 0) {
        suppressions.skipNextLine.erase(lineNumber);
        return true;
    }
    if (suppressions.checksToSkipByLine.find(LineAndCheckName(lineNumber, checkName)) != suppressions.checksToSkipByLine.cend())
        return true;

    return false;
}

void SuppressionManager::parseFile(FileID id, const SourceManager &sm, const clang::LangOptions &lo) const
{
    const unsigned hash = id.getHashValue();
    auto it = m_processedFileIDs.insert({hash, Suppressions()}).first;
    Suppressions &suppressions = (*it).second;

    bool invalid = false;
    auto buffer = clazy::getBuffer(sm, id, &invalid);
    if (invalid) {
        llvm::errs() << "SuppressionManager::parseFile: Invalid buffer ";
        if (buffer) {
            llvm::errs() << buffer->getBuffer() << "\n";
        }
        return;
    }

    auto lexer = GET_LEXER(id, buffer, sm, lo);
    lexer.SetCommentRetentionState(true);

    Token token;
    while (!lexer.LexFromRawLexer(token)) {
        if (token.getKind() == tok::comment) {
            std::string comment = Lexer::getSpelling(token, sm, lo);

            if (clazy::contains(comment, "clazy:skip")) {
                suppressions.skipEntireFile = true;
                return;
            }

            if (clazy::contains(comment, "NOLINTNEXTLINE")) {
                if (sm.getSpellingLineNumber(token.getLocation()) < 0) {
                    llvm::errs() << "SuppressionManager::parseFile: Invalid line number " << sm.getSpellingLineNumber(token.getLocation()) << "\n";
                    continue;
                }

                const int nextLineNumber = sm.getSpellingLineNumber(token.getLocation()) + 1;
                suppressions.skipNextLine.insert(nextLineNumber);
            }

            static std::regex rx(R"(clazy:excludeall=(.*?)(\s|$))");
            std::smatch match;
            if (regex_search(comment, match, rx) && match.size() > 1) {
                std::vector<std::string> checks = clazy::splitString(match[1], ',');
                suppressions.checksToSkip.insert(checks.cbegin(), checks.cend());
            }

            const int lineNumber = sm.getSpellingLineNumber(token.getLocation());
            if (lineNumber < 0) {
                llvm::errs() << "SuppressionManager::parseFile: Invalid line number " << lineNumber << "\n";
                continue;
            }

            static std::regex rx2(R"(clazy:exclude=(.*?)(\s|$))");
            if (regex_search(comment, match, rx2) && match.size() > 1) {
                std::vector<std::string> checks = clazy::splitString(match[1], ',');

                for (const std::string &checkName : checks) {
                    suppressions.checksToSkipByLine.insert(LineAndCheckName(lineNumber, checkName));
                }
            }
        }
    }
}
