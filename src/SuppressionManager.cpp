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
#include <stack>
#include <vector>

using namespace clang;

static llvm::Regex createRegexFromGlob(StringRef &Glob)
{
    std::string RegexText("^");
    StringRef MetaChars("()^$|*+?.[]\\{}"); // Copied from clang-tidy to keep parsing identical
    for (char C : Glob) {
        if (C == '*') {
            RegexText.push_back('.');
        } else if (MetaChars.contains(C)) {
            RegexText.push_back('\\');
        }
        RegexText.push_back(C);
    }
    RegexText.push_back('$');
    return {RegexText};
}

static std::string parseDisabledChecks(const std::string &comment, size_t parentPosition)
{
    if (parentPosition < comment.length() && comment.at(parentPosition) == '(') {
        const int parentEnd = comment.find(')', parentPosition);
        if (parentEnd == -1) {
            return ""; // Malformed
        }
        return comment.substr(parentPosition + 1, parentEnd - parentPosition - 1);
    }
    return "";
}

static std::vector<llvm::Regex> splitDisabledChecksText(const std::string &disableText)
{
    std::vector<llvm::Regex> disabledChecksRegexes;
    for (const auto &split : clazy::splitString(disableText, ',')) {
        if (split.find("clazy-") == 0) {
            StringRef sanitizedCheckName = llvm::StringRef(split).substr(strlen("clazy-")).rtrim();
            disabledChecksRegexes.push_back(createRegexFromGlob(sanitizedCheckName));
        }
    }
    return disabledChecksRegexes;
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

    const LineNumber lineNumber = sm.getSpellingLineNumber(loc);
    if (suppressions.skipNextLine.count(lineNumber) > 0) {
        suppressions.skipNextLine.erase(lineNumber);
        return true;
    }
    if (suppressions.checksToSkipByLine.find(LineAndCheckName(lineNumber, checkName)) != suppressions.checksToSkipByLine.cend())
        return true;

    if (auto it = suppressions.checkWildcardsToSkipByLine.find(lineNumber); it != suppressions.checkWildcardsToSkipByLine.end()) {
        return it->second.match(checkName);
    }

    for (const auto &rangeSuppression : suppressions.checkWildcardsRanges) {
        if (rangeSuppression.begin <= lineNumber && rangeSuppression.end >= lineNumber) {
            return rangeSuppression.checksToSkipWildcard.empty()
                || std::ranges::any_of(rangeSuppression.checksToSkipWildcard, [&checkName](const llvm::Regex &regex) {
                       return regex.match(checkName);
                   });
        }
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
    std::stack<std::pair<LineNumber, std::string>> checkRangeStack;

    Token token;
    while (!lexer.LexFromRawLexer(token)) {
        if (token.getKind() != tok::comment) {
            continue;
        }

        const LineNumber lineNumber = sm.getSpellingLineNumber(token.getLocation());
        const std::string comment = Lexer::getSpelling(token, sm, lo);

        const int foundNolint = comment.find("NOLINT");
        int foundNoLintNextLine = -1;
        if (foundNolint != -1) {
            // Reuse starting position of previous match
            if (comment.compare(foundNolint + strlen("NOLINT"), strlen("NEXTLINE"), "NEXTLINE") == 0) {
                foundNoLintNextLine = foundNolint + strlen("NOLINT");
            }

            if (const int idx = comment.find("NOLINTBEGIN", foundNolint); idx != -1) {
                const auto &beginChecks = parseDisabledChecks(comment, idx + strlen("NOLINTBEGIN"));
                checkRangeStack.push({lineNumber, beginChecks});
            } else if (const int idx = comment.find("NOLINTEND", foundNolint); idx != -1) {
                const std::string &endChecks = parseDisabledChecks(comment, idx + strlen("NOLINTEND"));
                if (checkRangeStack.empty() || checkRangeStack.top().second != endChecks) {
                    llvm::errs() << "unmatched 'NOLINTEND' comment without a previous 'NOLINTBEGIN' comment\n";
                } else {
                    const auto [beginLineNumber, disabledChecks] = checkRangeStack.top();
                    checkRangeStack.pop();
                    suppressions.checkWildcardsRanges.push_back(CheckBeginAndEnd{splitDisabledChecksText(disabledChecks), beginLineNumber, lineNumber});
                }

            } else {
                // NOLINT or NOLINTNEXTLINE
                const int lineNumberToSuppress = foundNoLintNextLine == -1 ? lineNumber : lineNumber + 1;
                auto disableText = parseDisabledChecks(comment, foundNolint + strlen("NOLINT") + (foundNoLintNextLine == -1 ? 0 : strlen("NEXTLINE")));
                if (disableText.empty()) {
                    suppressions.skipNextLine.insert(lineNumberToSuppress);
                } else {
                    for (llvm::Regex &split : splitDisabledChecksText(disableText)) {
                        suppressions.checkWildcardsToSkipByLine.insert({lineNumberToSuppress, std::move(split)});
                    }
                }
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
