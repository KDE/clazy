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

#ifndef CLAZY_ACCESS_SPECIFIER_MANAGER_H
#define CLAZY_ACCESS_SPECIFIER_MANAGER_H

#include "checkbase.h"

#include <clang/Frontend/CompilerInstance.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/Specifiers.h>
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

enum QtAccessSpecifierType
{
    QtAccessSpecifier_None,
    QtAccessSpecifier_Unknown,
    QtAccessSpecifier_Slot,
    QtAccessSpecifier_Signal,
    QtAccessSpecifier_Invokable
};

struct ClazyAccessSpecifier
{
    clang::SourceLocation loc;
    clang::AccessSpecifier accessSpecifier;
    QtAccessSpecifierType qtAccessSpecifier;
};

using ClazySpecifierList = std::vector<ClazyAccessSpecifier>;

class AccessSpecifierManager
{
public:
    explicit AccessSpecifierManager(const clang::CompilerInstance &ci);
    void VisitDeclaration(clang::Decl *decl);

    /**
     * Returns if a method is a signal, a slot, or neither.
     */
    QtAccessSpecifierType qtAccessSpecifierType(const clang::CXXMethodDecl*) const;

    /**
     * Returns if a method is scriptable (Q_SCRIPTABLE)
     */
    bool isScriptable(const clang::CXXMethodDecl*) const;

    /**
     * Returns a string representations of a Qt Access Specifier Type
     */
    llvm::StringRef qtAccessSpecifierTypeStr(QtAccessSpecifierType) const;

private:
    ClazySpecifierList &entryForClassDefinition(clang::CXXRecordDecl*);
    const clang::CompilerInstance &m_ci;
    const clang::CXXRecordDecl *classDefinitionForLoc(clang::SourceLocation loc) const;
    std::unordered_map<const clang::CXXRecordDecl*, ClazySpecifierList> m_specifiersMap;
    AccessSpecifierPreprocessorCallbacks *const m_preprocessorCallbacks;
};

#endif
