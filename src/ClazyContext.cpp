/*
    This file is part of the clazy static checker.

    Copyright (C) 2017 Sergio Martins <smartins@kde.org>

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
#include "checkmanager.h"
#include "ClazyContext.h"
#include "FixItExporter.h"
#include "PreProcessorVisitor.h"

#include <clang/AST/ParentMap.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Lex/PreprocessorOptions.h>
#include <clang/Rewrite/Frontend/FixItRewriter.h>
#include <llvm/Support/Regex.h>

#include <stdlib.h>

using namespace std;
using namespace clang;


ClazyContext::ClazyContext(const clang::CompilerInstance &compiler,
                           const string &headerFilter, const string &ignoreDirs,
                           string exportFixesFilename,
                           const std::vector<string> &translationUnitPaths, ClazyOptions opts)
    : ci(compiler)
    , astContext(ci.getASTContext())
    , sm(ci.getSourceManager())
    , m_noWerror(getenv("CLAZY_NO_WERROR") != nullptr) // Allows user to make clazy ignore -Werror
    , m_checksPromotedToErrors(CheckManager::instance()->checksAsErrors())
    , options(opts)
    , extraOptions(clazy::splitString(getenv("CLAZY_EXTRA_OPTIONS"), ','))
    , m_translationUnitPaths(translationUnitPaths)
{
    if (!headerFilter.empty())
        headerFilterRegex = std::unique_ptr<llvm::Regex>(new llvm::Regex(headerFilter));

    if (!ignoreDirs.empty())
        ignoreDirsRegex = std::unique_ptr<llvm::Regex>(new llvm::Regex(ignoreDirs));

    if (exportFixesEnabled()) {
        if (exportFixesFilename.empty()) {
            // Only clazy-standalone sets the filename by argument.
            // clazy plugin sets it automatically here:
            const FileEntry *fileEntry = sm.getFileEntryForID(sm.getMainFileID());
            exportFixesFilename = fileEntry->getName().str() + ".clazy.yaml";
        }

        const bool isClazyStandalone = !translationUnitPaths.empty();
        exporter = new FixItExporter(ci.getDiagnostics(), sm, ci.getLangOpts(),
                                     exportFixesFilename, isClazyStandalone);
    }
}

ClazyContext::~ClazyContext()
{
    //delete preprocessorVisitor; // we don't own it
    delete accessSpecifierManager;
    delete parentMap;

    static unsigned long count = 0;
    count++;

    if (exporter) {
        // With clazy-standalone we use the same YAML file for all translation-units, so only
        // write out the last one. With clazy-plugin there's a YAML file per translation unit.
        const bool isClazyPlugin = m_translationUnitPaths.empty();
        const bool isLast = count == m_translationUnitPaths.size();
        if (isLast || isClazyPlugin)
            exporter->Export();
        delete exporter;
    }

    preprocessorVisitor = nullptr;
    accessSpecifierManager = nullptr;
    parentMap = nullptr;
}

void ClazyContext::enableAccessSpecifierManager()
{
    if (!accessSpecifierManager && !usingPreCompiledHeaders())
        accessSpecifierManager = new AccessSpecifierManager(ci);
}

void ClazyContext::enablePreprocessorVisitor()
{
    if (!preprocessorVisitor && !usingPreCompiledHeaders())
        preprocessorVisitor = new PreProcessorVisitor(ci);
}

void ClazyContext::enableVisitallTypeDefs()
{
    // By default we only process decls from the .cpp file we're processing, not stuff included (for performance)
    /// But we might need to process all typedefs, not only the ones in our current .cpp files
    m_visitsAllTypeDefs = true;
}

bool ClazyContext::visitsAllTypedefs() const
{
    return m_visitsAllTypeDefs;
}

bool ClazyContext::isQt() const
{
    static const bool s_isQt = [this] {
                                   for (auto s : ci.getPreprocessorOpts().Macros) {
                                       if (s.first == "QT_CORE_LIB")
                                           return true;
                                   }
                                   return false;
                               } ();

    return s_isQt;
}
