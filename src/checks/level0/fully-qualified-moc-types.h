/*
  This file is part of the clazy static checker.

    Copyright (C) 2018 Sergio Martins <smartins@kde.org>

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

#ifndef CLAZY_FULLY_QUALIFIED_MOC_TYPES_H
#define CLAZY_FULLY_QUALIFIED_MOC_TYPES_H

#include "checkbase.h"

#include <clang/Basic/SourceLocation.h>

#include <vector>
#include <string>

class ClazyContext;
namespace clang {
class CXXMethodDecl;
class CXXRecordDecl;
class Decl;
class MacroInfo;
class Token;
}  // namespace clang

/**
 * See README-fully-qualified-moc-types.md for more info.
 */
class FullyQualifiedMocTypes
    : public CheckBase
{
public:
    explicit FullyQualifiedMocTypes(const std::string &name, ClazyContext *context);
    void VisitDecl(clang::Decl *) override;
private:
    bool isGadget(clang::CXXRecordDecl *record) const;
    bool handleQ_PROPERTY(clang::CXXMethodDecl *);
    void VisitMacroExpands(const clang::Token &MacroNameTok,
                           const clang::SourceRange &range, const clang::MacroInfo *minfo = nullptr) override;
    void registerQ_GADGET(clang::SourceLocation);
    bool typeIsFullyQualified(clang::QualType t, std::string &qualifiedTypeName, std::string &typeName) const;

    std::vector<clang::SourceLocation> m_qgadgetMacroLocations;
};
#endif
