/*
    SPDX-FileCopyrightText: 2016 Sergio Martins <smartins@kde.org>
    SPDX-FileCopyrightText: 2016 Klarälvdalens Datakonsult AB a KDAB Group company info@kdab.com

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLAZY_ACCESS_SPECIFIER_MANAGER_H
#define CLAZY_ACCESS_SPECIFIER_MANAGER_H

#include "checkbase.h"

#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/Specifiers.h>
#include <clang/Frontend/CompilerInstance.h>
#include <llvm/ADT/StringRef.h>

#include <unordered_map>
#include <vector>

/*
   clang supports "public", "private" and "protected"
   This is a small wrapper to have support for "public slots", "signals", etc. so you can easily
   query if a method is a signal or slot

   This works in two steps, first the pre-processor kicks in and we use a PPCallbacks class to catch
   Q_SLOTS and Q_SIGNALS, then the normal access specifiers come in from the AST, inside VisitDeclaration().

   After that we just need to merge the two lists, and sort by source location. All the info is kept
   inside m_specifiersMap, which is indexed by the class definition.
*/

namespace clang
{
class SourceManager;
class LangOptions;
class Decl;
class CXXRecordDecl;
class SourceLocation;
class CXXMethodDecl;
class CompilerInstance;
}

class AccessSpecifierPreprocessorCallbacks;

enum QtAccessSpecifierType { QtAccessSpecifier_None, QtAccessSpecifier_Unknown, QtAccessSpecifier_Slot, QtAccessSpecifier_Signal, QtAccessSpecifier_Invokable };

struct ClazyAccessSpecifier {
    clang::SourceLocation loc;
    clang::AccessSpecifier accessSpecifier;
    QtAccessSpecifierType qtAccessSpecifier;
};

using ClazySpecifierList = std::vector<ClazyAccessSpecifier>;

class AccessSpecifierManager
{
public:
    explicit AccessSpecifierManager(const clang::SourceManager &manager, const clang::LangOptions lo, clang::Preprocessor &pi, bool exportFixesEnabled);
    void VisitDeclaration(clang::Decl *decl);

    /**
     * Returns if a method is a signal, a slot, or neither.
     */
    QtAccessSpecifierType qtAccessSpecifierType(const clang::CXXMethodDecl *) const;

    /**
     * Returns if a method is scriptable (Q_SCRIPTABLE)
     */
    bool isScriptable(const clang::CXXMethodDecl *) const;

    /**
     * Returns a string representations of a Qt Access Specifier Type
     */
    llvm::StringRef qtAccessSpecifierTypeStr(QtAccessSpecifierType) const;

    clang::SourceLocation firstLocationOfSection(clang::AccessSpecifier specifier, clang::CXXRecordDecl *decl) const;

private:
    ClazySpecifierList &entryForClassDefinition(clang::CXXRecordDecl *);
    const clang::CXXRecordDecl *classDefinitionForLoc(clang::SourceLocation loc) const;
    const clang::SourceManager &m_sm;
    const clang::LangOptions m_lo;
    std::unordered_map<const clang::CXXRecordDecl *, ClazySpecifierList> m_specifiersMap;
    AccessSpecifierPreprocessorCallbacks *const m_preprocessorCallbacks;
    const bool m_fixitsEnabled;
    bool m_visitsNonQObjects = false;
};

#endif
