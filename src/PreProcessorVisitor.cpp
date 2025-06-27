/*
    SPDX-FileCopyrightText: 2017 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "PreProcessorVisitor.h"
#include "MacroUtils.h"

#include <clang/Basic/IdentifierTable.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Lex/MacroInfo.h>
#include <clang/Lex/PPCallbacks.h>
#include <clang/Lex/Preprocessor.h>
#include <clang/Lex/Token.h>
#include <llvm/ADT/ArrayRef.h>

#include <cstdlib>
#include <memory>

using namespace clang;

PreProcessorVisitor::PreProcessorVisitor(Preprocessor &pp)
    : clang::PPCallbacks()
    , m_pp(pp)

{
    pp.addPPCallbacks(std::unique_ptr<PPCallbacks>(this));

    // This catches -DQT_NO_KEYWORDS passed to compiler. In MacroExpands() we catch when defined via in code
    m_isQtNoKeywords = clazy::isPredefined(pp.getPreprocessorOpts(), "QT_NO_KEYWORDS");
}

bool PreProcessorVisitor::isBetweenQtNamespaceMacros(SourceLocation loc)
{
    if (loc.isInvalid()) {
        return false;
    }

    const auto &sm = m_pp.getSourceManager();
    if (loc.isMacroID()) {
        loc = sm.getExpansionLoc(loc);
    }

    uint fileId = sm.getFileID(loc).getHashValue();

    std::vector<SourceRange> &pairs = m_q_namespace_macro_locations[fileId];
    for (SourceRange &pair : pairs) {
        if (pair.getBegin().isInvalid() || pair.getEnd().isInvalid()) {
            // llvm::errs() << "PreProcessorVisitor::isBetweenQtNamespaceMacros Found invalid location\n";
            continue; // shouldn't happen
        }

        if (sm.isBeforeInSLocAddrSpace(pair.getBegin(), loc) && sm.isBeforeInSLocAddrSpace(loc, pair.getEnd())) {
            return true;
        }
    }

    return false;
}

bool PreProcessorVisitor::hasInclude(const std::string &fileName, bool IsAngled) const
{
    auto it = std::find_if(m_includeInfo.cbegin(), m_includeInfo.cend(), [&](const IncludeInfo &info) {
        return info.fileName == fileName && info.IsAngled == IsAngled;
    });
    return (it != m_includeInfo.cend());
}

SourceLocation PreProcessorVisitor::endOfIncludeSection() const
{
    if (m_includeInfo.empty()) {
        return {};
    }
    return m_includeInfo.back().filenameRange.getEnd();
}

std::string PreProcessorVisitor::getTokenSpelling(const MacroDefinition &def) const
{
    if (!def) {
        return {};
    }

    MacroInfo *info = def.getMacroInfo();
    if (!info) {
        return {};
    }

    std::string result;
    for (const auto &tok : info->tokens()) {
        result += m_pp.getSpelling(tok);
    }

    return result;
}

void PreProcessorVisitor::updateQtVersion()
{
    if (m_qtMajorVersion == -1 || m_qtPatchVersion == -1 || m_qtMinorVersion == -1) {
        m_qtVersion = -1;
    } else {
        m_qtVersion = m_qtPatchVersion + m_qtMinorVersion * 100 + m_qtMajorVersion * 10000;
    }
}

void PreProcessorVisitor::handleQtNamespaceMacro(SourceLocation loc, StringRef name)
{
    const bool isBegin = name == "QT_BEGIN_NAMESPACE";
    uint fileId = m_pp.getSourceManager().getFileID(loc).getHashValue();
    std::vector<SourceRange> &pairs = m_q_namespace_macro_locations[fileId];

    if (isBegin) {
        pairs.push_back(SourceRange(loc, {}));
    } else {
        if (pairs.empty()) {
            // llvm::errs() << "FOO Received end!!";
        } else {
            SourceRange &range = pairs[pairs.size() - 1];
            if (range.getBegin().isInvalid()) {
                // llvm::errs() << "FOO Error received end before a begin\n";
            } else {
                range.setEnd(loc);
            }
        }
    }
}

static int stringToNumber(const std::string &str)
{
    if (str.empty()) {
        return -1;
    }

    return atoi(str.c_str());
}

void PreProcessorVisitor::MacroExpands(const Token &MacroNameTok, const MacroDefinition &def, SourceRange range, const MacroArgs *)
{
    IdentifierInfo *ii = MacroNameTok.getIdentifierInfo();
    if (!ii) {
        return;
    }

    if (ii->getName() == "QT_BEGIN_NAMESPACE" || ii->getName() == "QT_END_NAMESPACE") {
        handleQtNamespaceMacro(range.getBegin(), ii->getName());
        return;
    }

    if (!m_isQtNoKeywords && ii->getName() == "QT_NO_KEYWORDS") {
        m_isQtNoKeywords = true;
        return;
    }

    if (m_qtVersion != -1) {
        return;
    }

    auto name = ii->getName();
    if (name == "QT_VERSION_MAJOR") {
        m_qtMajorVersion = stringToNumber(getTokenSpelling(def));
        updateQtVersion();
    }

    if (name == "QT_VERSION_MINOR") {
        m_qtMinorVersion = stringToNumber(getTokenSpelling(def));
        updateQtVersion();
    }

    if (name == "QT_VERSION_PATCH") {
        m_qtPatchVersion = stringToNumber(getTokenSpelling(def));
        updateQtVersion();
    }
}

void PreProcessorVisitor::InclusionDirective(clang::SourceLocation,
                                             const clang::Token &,
                                             clang::StringRef FileName,
                                             bool IsAngled,
                                             clang::CharSourceRange FilenameRange,
                                             clazy::OptionalFileEntryRef,
                                             clang::StringRef,
                                             clang::StringRef,
                                             const clang::Module *,
                                             bool,
                                             clang::SrcMgr::CharacteristicKind)
{
    if (m_pp.isInPrimaryFile() && !clazy::endsWith(FileName.str(), ".moc")) {
        m_includeInfo.push_back(IncludeInfo{FileName, IsAngled, FilenameRange});
    }
}
