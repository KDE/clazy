/*
  This file is part of the clazy static checker.

  Copyright (C) 2016 Sergio Martins <smartins@kde.org>
  Copyright (C) 2016 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com

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

#if !defined(IS_OLD_CLANG)

#include "StringUtils.h"
#include "HierarchyUtils.h"

#include <clang/Basic/SourceManager.h>
#include <clang/Parse/Parser.h>
#include <clang/AST/DeclCXX.h>

using namespace clang;
using namespace std;

static bool accessSpecifierCompare(const ClazyAccessSpecifier &lhs, const ClazyAccessSpecifier &rhs,
                                   const SourceManager &sm)
{
    if (lhs.loc.isMacroID() || rhs.loc.isMacroID()) {
        // Q_SIGNALS is special because it hides a "public", which is expanded by this macro.
        // That means that both the Q_SIGNALS macro and the "public" will have the same source location.
        // We do want the "public" to appear before, so check if one has a macro id on it.

        SourceLocation realLHSLoc = sm.getFileLoc(lhs.loc);
        SourceLocation realRHSLoc = sm.getFileLoc(rhs.loc);
        if (realLHSLoc == realRHSLoc) {
            return lhs.loc.isMacroID();
        } else {
            return realLHSLoc < realRHSLoc;
        }
    }

    return lhs.loc < rhs.loc;
};

static void sorted_insert(ClazySpecifierList &v, const ClazyAccessSpecifier &item, const clang::SourceManager &sm)
{
    auto pred = [&sm] (const ClazyAccessSpecifier &lhs, const ClazyAccessSpecifier &rhs) {
        return accessSpecifierCompare(lhs, rhs, sm);
    };
    v.insert(std::upper_bound(v.begin(), v.end(), item, pred), item);
}

#if !(LLVM_VERSION_MAJOR == 3 && LLVM_VERSION_MINOR == 6)
class AccessSpecifierPreprocessorCallbacks : public clang::PPCallbacks
{
    AccessSpecifierPreprocessorCallbacks(const AccessSpecifierPreprocessorCallbacks &) = delete;
public:
    AccessSpecifierPreprocessorCallbacks(AccessSpecifierManager *q, const clang::CompilerInstance &ci)
        : clang::PPCallbacks()
        , q(q)
        , m_ci(ci)
    {
        m_qtAccessSpecifiers.reserve(30); // bootstrap it
    }

    void MacroExpands(const Token &MacroNameTok, const MacroDefinition &,
                      SourceRange range, const MacroArgs *) override
    {
        IdentifierInfo *ii = MacroNameTok.getIdentifierInfo();
        if (!ii)
            return;

        auto name = ii->getName();
        const bool isSlots = name == "slots" || name == "Q_SLOTS";
        const bool isSignals = isSlots ? false : (name == "signals" || name == "Q_SIGNALS");
        if (!isSlots && !isSignals)
            return;

        const SourceLocation loc = range.getBegin();
        if (loc.isMacroID())
            return;

        QtAccessSpecifierType qtAccessSpecifier = isSlots ? QtAccessSpecifier_Slot
                                                          : QtAccessSpecifier_Signal;
        m_qtAccessSpecifiers.push_back( { loc, clang::AS_none, qtAccessSpecifier } );
    }

    AccessSpecifierManager *const q;
    const CompilerInstance &m_ci;
    ClazySpecifierList m_qtAccessSpecifiers;
};
#endif

AccessSpecifierManager::AccessSpecifierManager(const clang::CompilerInstance &ci)
    : m_ci(ci)
    , m_preprocessorCallbacks(new AccessSpecifierPreprocessorCallbacks(this, ci))
{
    Preprocessor &pi = m_ci.getPreprocessor();
    pi.addPPCallbacks(std::unique_ptr<PPCallbacks>(m_preprocessorCallbacks));
}

ClazySpecifierList& AccessSpecifierManager::entryForClassDefinition(CXXRecordDecl *classDecl)
{
    ClazySpecifierList &specifiers = m_specifiersMap[classDecl];
    return specifiers;
}

CXXRecordDecl *AccessSpecifierManager::classDefinitionForLoc(SourceLocation loc)
{
    for (auto it : m_specifiersMap) {
        CXXRecordDecl *record = it.first;
        if (record->getLocStart() < loc && loc < record->getLocEnd())
            return record;
    }
    return nullptr;
}

void AccessSpecifierManager::VisitDeclaration(Decl *decl)
{
    auto record = dyn_cast<CXXRecordDecl>(decl);
    auto accessSpec = dyn_cast<AccessSpecDecl>(decl);

    if (!record && !accessSpec)
        return;

    const auto &sm = m_ci.getSourceManager();

    if (record) {
        // We got a new record, lets fetch signals and slots that the pre-processor gathered

        ClazySpecifierList &specifiers = entryForClassDefinition(record);

        auto it = m_preprocessorCallbacks->m_qtAccessSpecifiers.begin();
        while (it != m_preprocessorCallbacks->m_qtAccessSpecifiers.end()) {
            if (classDefinitionForLoc((*it).loc) == record) {
                sorted_insert(specifiers, *it, sm);
                it = m_preprocessorCallbacks->m_qtAccessSpecifiers.erase(it);
            } else {
                ++it;
            }
        }
    } else if (accessSpec) {

        DeclContext *declContext = accessSpec->getDeclContext();
        auto record = dyn_cast<CXXRecordDecl>(declContext);
        if (!record) {
            llvm::errs() << "Received access specifier without class definition\n";
            return;
        }

        ClazySpecifierList &specifiers = entryForClassDefinition(record);
        sorted_insert(specifiers, {decl->getLocStart(), accessSpec->getAccess(), QtAccessSpecifier_None }, sm);
    }
}

QtAccessSpecifierType AccessSpecifierManager::qtAccessSpecifierType(CXXMethodDecl *method) const
{
    if (!method)
        return QtAccessSpecifier_Unknown;

    CXXRecordDecl *record = method->getParent();
    if (!record || isa<clang::ClassTemplateSpecializationDecl>(record))
        return QtAccessSpecifier_None;

    auto it = m_specifiersMap.find(record);
    if (it == m_specifiersMap.cend())
        return QtAccessSpecifier_Unknown;

    const ClazySpecifierList &accessSpecifiers = it->second;

    auto pred = [this] (const ClazyAccessSpecifier &lhs, const ClazyAccessSpecifier &rhs) {
        return accessSpecifierCompare(lhs, rhs, m_ci.getSourceManager());
    };

    const ClazyAccessSpecifier dummy = { method->getLocStart(), // we're only interested in the location
                                         /*dummy*/ clang::AS_none,
                                         /*dummy*/ QtAccessSpecifier_None };
    auto i = std::upper_bound(accessSpecifiers.cbegin(), accessSpecifiers.cend(), dummy, pred);
    if (i == accessSpecifiers.cbegin())
        return QtAccessSpecifier_None;


    --i; // One before the upper bound is the last access specifier before our method
    return (*i).qtAccessSpecifier;
}

#endif
