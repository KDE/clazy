/*
    SPDX-FileCopyrightText: 2016 Sergio Martins <smartins@kde.org>
    SPDX-FileCopyrightText: 2025 Alexander Lohnau <alexander.lohnau@gmx.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "SuppressionManager.h"
#include "clazy_stl.h"

#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Basic/TokenKinds.h>
#include <clang/Lex/Lexer.h>
#include <clang/Lex/Token.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/raw_ostream.h>

#include <regex>
#include <vector>

using namespace clang;

static llvm::Regex createRegexFromGlob(StringRef &Glob)
{
    SmallString<128> RegexText("^");
    StringRef MetaChars("()^$|*+?.[]\\{}");
    for (char C : Glob) {
        if (C == '*')
            RegexText.push_back('.');
        else if (MetaChars.contains(C))
            RegexText.push_back('\\');
        RegexText.push_back(C);
    }
    RegexText.push_back('$');
    return {RegexText.str()};
}

SuppressionManager::SuppressionManager() = default;

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

    if (auto it = suppressions.checkWildcardsToSkipByLine.find(lineNumber); it != suppressions.checkWildcardsToSkipByLine.end()) {
        return it->second.match(checkName);
    }

    return false;
}

void SuppressionManager::parseFile(FileID id, const SourceManager &sm, const clang::LangOptions &lo) const
{
    const unsigned hash = id.getHashValue();
    auto it = m_processedFileIDs.insert({hash, Suppressions()}).first;
    Suppressions &suppressions = (*it).second;

    auto buffer = sm.getBufferOrNone(id);
    if (!buffer.has_value()) {
        llvm::errs() << "SuppressionManager::parseFile: Invalid buffer ";
        if (buffer) {
            llvm::errs() << buffer->getBuffer() << "\n";
        }
        return;
    }

    clang::Lexer lexer(id, buffer.value(), sm, lo);
    lexer.SetCommentRetentionState(true);

    Token token;
    while (!lexer.LexFromRawLexer(token)) {
        if (token.getKind() != tok::comment) {
            continue;
        }

        const int lineNumber = sm.getSpellingLineNumber(token.getLocation());
        const std::string comment = Lexer::getSpelling(token, sm, lo);
        if (lineNumber < 0) {
            llvm::errs() << "SuppressionManager::parseFile: Invalid line number " << lineNumber << "\n";
            continue;
        }

        const int foundNolint = comment.find("NOLINT");
        int foundNoLintNextLine = -1;
        // Reuse starting position of previous match
        if (foundNolint != -1 && comment.compare(foundNolint + strlen("NOLINT"), strlen("NEXTLINE"), "NEXTLINE") == 0) {
            foundNoLintNextLine = foundNolint + strlen("NOLINT");
        }

        if (foundNolint != -1) {
            const int lineNumberToSuppress = foundNoLintNextLine == -1 ? lineNumber : lineNumber + 1;
            const size_t parentPosition = foundNoLintNextLine == -1 ? foundNolint : foundNoLintNextLine + strlen("NEXTLINE");
            if (parentPosition < comment.length() && comment.at(parentPosition) == '(') {
                const int parentEnd = comment.find(')', parentPosition);
                if (parentEnd == -1) {
                    continue; // Malformed
                }
                const std::string disableText = comment.substr(parentPosition + 1, parentEnd - parentPosition - 1);
                for (const auto &split : clazy::splitString(disableText, ',')) {
                    if (split.find("clazy-") == 0) {
                        StringRef sanitizedCheckName = llvm::StringRef(split).substr(strlen("clazy-")).rtrim();
                        suppressions.checkWildcardsToSkipByLine.insert({lineNumberToSuppress, createRegexFromGlob(sanitizedCheckName)});
                    }
                }
            } else {
                suppressions.skipNextLine.insert(lineNumberToSuppress);
            }
        }

        const auto startIdx = comment.find("clazy:");
        if (startIdx == std::string::npos) {
            continue; // Early return, no need to look at any regex
        }

        if (clazy::contains(comment, "clazy:skip")) {
            suppressions.skipEntireFile = true;
            return;
        }

        static const std::regex rx_all("clazy:excludeall=([^\\s]+)");
        static const std::regex rx_current("clazy:exclude=([^\\s]+)");
        static const std::regex rx_next("clazy:exclude-next-line=([^\\s]+)");

        const auto startIt = comment.begin() + startIdx;
        const auto endIt = comment.end();
        std::smatch match;
        if (std::regex_search(startIt, endIt, match, rx_all)) {
            std::vector<std::string> checks = clazy::splitString(match[1], ',');
            suppressions.checksToSkip.insert(checks.cbegin(), checks.cend());
        } else if (std::regex_search(startIt, endIt, match, rx_current)) {
            std::vector<std::string> checks = clazy::splitString(match[1], ',');
            for (const std::string &checkName : checks) {
                suppressions.checksToSkipByLine.insert(LineAndCheckName(lineNumber, checkName));
            }
        } else if (std::regex_search(startIt, endIt, match, rx_next)) {
            std::vector<std::string> checks = clazy::splitString(match[1], ',');
            for (const std::string &checkName : checks) {
                suppressions.checksToSkipByLine.insert(LineAndCheckName(lineNumber + 1, checkName));
            }
        }
    }
}
