/*
    SPDX-FileCopyrightText: 2016 Sergio Martins <smartins@kde.org>
    SPDX-FileCopyrightText: 2025 Alexander Lohnau <alexander.lohnau@gmx.de>

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

#include <regex>
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
    if (it == m_processedFileIDs.cend()) {
        parseFile(fileID, sm, lo);
        it = m_processedFileIDs.find(fileID.getHashValue());
    }

    Suppressions &suppressions = it->second;

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
            const std::string comment = Lexer::getSpelling(token, sm, lo);

            if (clazy::contains(comment, "clazy:skip")) {
                suppressions.skipEntireFile = true;
                return;
            }

            if (clazy::contains(comment, "NOLINTNEXTLINE")) {
                bool invalid = false;
                const int nextLineNumber = sm.getSpellingLineNumber(token.getLocation(), &invalid) + 1;
                if (invalid) {
                    llvm::errs() << "SuppressionManager::parseFile: Invalid line number for token location where NOLINTNEXTLINE was found\n";
                    continue;
                }

                suppressions.skipNextLine.insert(nextLineNumber);
            }

            if (!clazy::contains(comment, "clazy:")) {
                continue; // Early return, no need to look at any regex
            }

            static std::regex rx_all("clazy:excludeall=([^\\s]+)");
            std::smatch match;
            if (regex_search(comment, match, rx_all)) {
                std::vector<std::string> checks = clazy::splitString(match[1], ',');
                suppressions.checksToSkip.insert(checks.cbegin(), checks.cend());
            }

            const int lineNumber = sm.getSpellingLineNumber(token.getLocation());
            if (lineNumber < 0) {
                llvm::errs() << "SuppressionManager::parseFile: Invalid line number " << lineNumber << "\n";
                continue;
            }

            static std::regex rx_current("clazy:exclude=([^\\s]+)");
            if (regex_search(comment, match, rx_current)) {
                std::vector<std::string> checks = clazy::splitString(match[1], ',');
                for (const std::string &checkName : checks) {
                    suppressions.checksToSkipByLine.insert(LineAndCheckName(lineNumber, checkName));
                }
            }
            static std::regex rx_next("clazy:exclude-next-line=([^\\s]+)");
            if (regex_search(comment, match, rx_next)) {
                std::vector<std::string> checks = clazy::splitString(match[1], ',');
                for (const std::string &checkName : checks) {
                    suppressions.checksToSkipByLine.insert(LineAndCheckName(lineNumber + 1, checkName));
                }
            }
        }
    }
}
