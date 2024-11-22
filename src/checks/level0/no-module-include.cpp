/*
    SPDX-FileCopyrightText: 2023 Johnny Jazeix <jazeix@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "no-module-include.h"
#include "ClazyContext.h"
#include "PreProcessorVisitor.h"

#include <clang/Basic/LLVM.h>

#include <vector>

using namespace clang;

NoModuleInclude::NoModuleInclude(const std::string &name, ClazyContext *context)
    : CheckBase(name, context)
    , m_modulesList{
          "Core",
          "Gui",
          "Qml",
          "QmlModels",
          "Test",
          "Network",
          "DBus",
          "Quick",
          "Svg",
          "Widgets",
          "Xml",
          "Concurrent",
          "Multimedia",
          "Sql",
          "PrintSupport",
          "NetworkAuth",
          "QmlBuiltins",
          "QmlIntegration",
      }
{
    for (const std::string &module : m_modulesList) {
        m_filesToIgnore.emplace_back("Qt" + module + "Depends");
    }
    enablePreProcessorCallbacks();
}

void NoModuleInclude::VisitInclusionDirective(clang::SourceLocation HashLoc,
                                              const clang::Token & /*IncludeTok*/,
                                              clang::StringRef FileName,
                                              bool /*IsAngled*/,
                                              clang::CharSourceRange FilenameRange,
                                              clazy::OptionalFileEntryRef /*File*/,
                                              clang::StringRef /*SearchPath*/,
                                              clang::StringRef /*RelativePath*/,
#if LLVM_VERSION_MAJOR >= 19
                                              const clang::Module * /*SuggestedModule*/,
                                              bool /*ModuleImported*/,
#else
                                              const clang::Module * /*Imported*/,
#endif
                                              clang::SrcMgr::CharacteristicKind /*FileType*/)
{
    if (shouldIgnoreFile(HashLoc)) {
        return;
    }

    for (const std::string &module : m_modulesList) {
        if (module == "DBus") { // Avoid false positive for generated files
            if (const auto fileEntry = sm().getFileEntryRefForID(sm().getFileID(HashLoc))) {
                llvm::StringRef fileName = fileEntry->getName();
                llvm::ErrorOr<std::unique_ptr<llvm::MemoryBuffer>> fileBuffer = llvm::MemoryBuffer::getFile(fileName);
                if (fileBuffer) {
                    llvm::StringRef fileContent = fileBuffer.get()->getBuffer();
                    if (clazy::startsWith(fileContent, "/*\n * This file was generated by qdbusxml2cpp")
                        || clazy::startsWith(fileContent, "/*\r\n * This file was generated by qdbusxml2cpp")) {
                        continue;
                    }
                }
            }
        }

        const std::string qtModule = "Qt" + module;
        if (FileName.str() == qtModule + "/" + qtModule || FileName.str() == qtModule) {
            emitWarning(FilenameRange.getAsRange().getBegin(), "Module " + qtModule + " should not be included directly");
        }
    }
}
