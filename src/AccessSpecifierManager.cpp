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
#include "QtUtils.h"
#include "SourceCompatibilityHelpers.h"
#include "Utils.h"

#include <clang/Basic/SourceManager.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/DeclBase.h>
#include <clang/AST/DeclTemplate.h>
#include <clang/Basic/IdentifierTable.h>
#include <clang/Basic/LLVM.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Lex/PPCallbacks.h>
#include <clang/Lex/Preprocessor.h>
#include <clang/Lex/Token.h>
#include <llvm/Support/Casting.h>

#include <algorithm>
#include <memory>
#include <utility>

namespace clang {
class MacroArgs;
class MacroDefinition;
}  // namespace clang

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
}

static void sorted_insert(ClazySpecifierList &v, const ClazyAccessSpecifier &item, const clang::SourceManager &sm)
{
    auto pred = [&sm] (const ClazyAccessSpecifier &lhs, const ClazyAccessSpecifier &rhs) {
        return accessSpecifierCompare(lhs, rhs, sm);
    };
    v.insert(std::upper_bound(v.begin(), v.end(), item, pred), item);
}

class AccessSpecifierPreprocessorCallbacks
    : public clang::PPCallbacks
{
    AccessSpecifierPreprocessorCallbacks(const AccessSpecifierPreprocessorCallbacks &) = delete;
public:
    AccessSpecifierPreprocessorCallbacks(const clang::CompilerInstance &ci)
        : clang::PPCallbacks()
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

        const bool isSlot = (isSlots || isSignals) ? false : name == "Q_SLOT";
        const bool isSignal = (isSlots || isSignals || isSlot) ? false : name == "Q_SIGNAL";
        const bool isInvokable = (isSlots || isSignals || isSlot || isSignal) ? false : name == "Q_INVOKABLE";
        const bool isScriptable = (isSlot || isSignal || isInvokable) ? false : name == "Q_SCRIPTABLE";
        if (!isSlots && !isSignals && !isSlot && !isSignal && !isInvokable && !isScriptable)
            return;

        SourceLocation loc = range.getBegin();
        if (loc.isMacroID())
            return;

        if (isSignals || isSlots) {
            QtAccessSpecifierType qtAccessSpecifier = isSlots ? QtAccessSpecifier_Slot
                                                              : QtAccessSpecifier_Signal;
            m_qtAccessSpecifiers.push_back( { loc, clang::AS_none, qtAccessSpecifier } );
        } else {
            // Get the location of the method declaration, so we can compare directly when we visit methods
            loc = Utils::locForNextToken(loc, m_ci.getSourceManager(), m_ci.getLangOpts());
            if (loc.isInvalid())
                return;
            if (isSignal) {
                m_individualSignals.push_back(loc.getRawEncoding());
            } else if (isSlot) {
                m_individualSlots.push_back(loc.getRawEncoding());
            } else if (isInvokable) {
                m_invokables.push_back(loc.getRawEncoding());
            } else if (isScriptable) {
                m_scriptables.push_back(loc.getRawEncoding());
            }
        }
    }

    vector<unsigned> m_individualSignals; // Q_SIGNAL
    vector<unsigned> m_individualSlots;   // Q_SLOT
    vector<unsigned> m_invokables; // Q_INVOKABLE
    vector<unsigned> m_scriptables; // Q_SCRIPTABLE
    const CompilerInstance &m_ci;
    ClazySpecifierList m_qtAccessSpecifiers;
};

AccessSpecifierManager::AccessSpecifierManager(const clang::CompilerInstance &ci)
    : m_ci(ci)
    , m_preprocessorCallbacks(new AccessSpecifierPreprocessorCallbacks(ci))
{
    Preprocessor &pi = m_ci.getPreprocessor();
    pi.addPPCallbacks(unique_ptr<PPCallbacks>(m_preprocessorCallbacks));
}

ClazySpecifierList& AccessSpecifierManager::entryForClassDefinition(CXXRecordDecl *classDecl)
{
    ClazySpecifierList &specifiers = m_specifiersMap[classDecl];
    return specifiers;
}

const CXXRecordDecl *AccessSpecifierManager::classDefinitionForLoc(SourceLocation loc) const
{
    for (const auto &it : m_specifiersMap) {
        const CXXRecordDecl *record = it.first;
        if (clazy::getLocStart(record) < loc && loc < clazy::getLocEnd(record))
            return record;
    }
    return nullptr;
}

void AccessSpecifierManager::VisitDeclaration(Decl *decl)
{
    auto record = dyn_cast<CXXRecordDecl>(decl);
    if (!clazy::isQObject(record))
        return;

    const auto &sm = m_ci.getSourceManager();

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

    // Now lets add the normal C++ access specifiers (public, private etc.)

    for (auto d : record->decls()) {
        auto accessSpec = dyn_cast<AccessSpecDecl>(d);
        if (!accessSpec || accessSpec->getDeclContext() != record)
            continue;
        ClazySpecifierList &specifiers = entryForClassDefinition(record);
        sorted_insert(specifiers, {clazy::getLocStart(accessSpec), accessSpec->getAccess(), QtAccessSpecifier_None }, sm);
    }
}

QtAccessSpecifierType AccessSpecifierManager::qtAccessSpecifierType(const CXXMethodDecl *method) const
{
    if (!method || clazy::getLocStart(method).isMacroID())
        return QtAccessSpecifier_Unknown;

    // We want the declaration that's inside class {}, not the ones that are also a method definition
    // and possibly outside the class
    method = method->getCanonicalDecl();

    const CXXRecordDecl *record = method->getParent();
    if (!record || isa<clang::ClassTemplateSpecializationDecl>(record))
        return QtAccessSpecifier_None;

    const SourceLocation methodLoc = clazy::getLocStart(method);

    // Process Q_SIGNAL:
    for (auto signalLoc : m_preprocessorCallbacks->m_individualSignals) {
        if (signalLoc == methodLoc.getRawEncoding())
            return QtAccessSpecifier_Signal;
    }

    // Process Q_SLOT:
    for (auto slotLoc : m_preprocessorCallbacks->m_individualSlots) {
        if (slotLoc == methodLoc.getRawEncoding())
            return QtAccessSpecifier_Slot;
    }

    // Process Q_INVOKABLE:
    for (auto loc : m_preprocessorCallbacks->m_invokables) {
        if (loc == methodLoc.getRawEncoding())
            return QtAccessSpecifier_Invokable;
    }

    // Process Q_SLOTS and Q_SIGNALS:

    auto it = m_specifiersMap.find(record);
    if (it == m_specifiersMap.cend())
        return QtAccessSpecifier_None;

    const ClazySpecifierList &accessSpecifiers = it->second;

    auto pred = [this] (const ClazyAccessSpecifier &lhs, const ClazyAccessSpecifier &rhs) {
        return accessSpecifierCompare(lhs, rhs, m_ci.getSourceManager());
    };

    const ClazyAccessSpecifier dummy = { methodLoc, // we're only interested in the location
                                         /*dummy*/ clang::AS_none,
                                         /*dummy*/ QtAccessSpecifier_None };
    auto i = std::upper_bound(accessSpecifiers.cbegin(), accessSpecifiers.cend(), dummy, pred);
    if (i == accessSpecifiers.cbegin())
        return QtAccessSpecifier_None;


    --i; // One before the upper bound is the last access specifier before our method
    return (*i).qtAccessSpecifier;
}

bool AccessSpecifierManager::isScriptable(const CXXMethodDecl *method) const
{
    if (!method)
        return false;

    const SourceLocation methodLoc = clazy::getLocStart(method);
     if (methodLoc.isMacroID())
         return false;

    for (auto loc : m_preprocessorCallbacks->m_scriptables) {
        if (loc == methodLoc.getRawEncoding())
            return true;
    }

    return false;
}

llvm::StringRef AccessSpecifierManager::qtAccessSpecifierTypeStr(QtAccessSpecifierType t) const
{
    switch (t) {
    case QtAccessSpecifier_None:
    case QtAccessSpecifier_Unknown:
        return "";
    case QtAccessSpecifier_Slot:
        return "slot";
    case QtAccessSpecifier_Signal:
        return "signal";
    case QtAccessSpecifier_Invokable:
        return "invokable";
    }

    return "";
}
