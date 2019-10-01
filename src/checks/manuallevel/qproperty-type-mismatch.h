/*
  This file is part of the clazy static checker.

    Copyright (C) 2019 Jean-Michaël Celerier <jeanmichael.celerier@gmail.com>

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

#ifndef CLAZY_QPROPERTY_TYPE_MISMATCH_H
#define CLAZY_QPROPERTY_TYPE_MISMATCH_H

#include "checkbase.h"

#include <clang/Basic/SourceLocation.h>

#include <vector>
#include <string>
#include <unordered_map>

class ClazyContext;
namespace clang {
class CXXMethodDecl;
class FieldDecl;
class Decl;
class MacroInfo;
class Token;
class TypeAliasDecl;
}  // namespace clang

/**
 * See README-qproperty-type-mismatch.md for more info.
 */
class QPropertyTypeMismatch
    : public CheckBase
{
public:
    explicit QPropertyTypeMismatch(const std::string &name, ClazyContext *context);
    void VisitDecl(clang::Decl *) override;
private:
    void VisitMethod(const clang::CXXMethodDecl &);
    void VisitField(const clang::FieldDecl &);
    void VisitTypedef(const clang::TypedefNameDecl *);

    void VisitMacroExpands(const clang::Token &MacroNameTok,
                           const clang::SourceRange &range, const clang::MacroInfo *minfo = nullptr) override;

    struct Property
    {
        clang::SourceLocation loc;
        bool member{};
        std::string name;
        std::string type;
        std::string read;
        std::string write;
        std::string notify;
    };

    std::vector<Property> m_qproperties;
    std::string cleanupType(clang::QualType type, bool unscoped = false) const;
    void checkMethodAgainstProperty(const Property &prop, const clang::CXXMethodDecl &method, const std::string &methodName);
    void checkFieldAgainstProperty(const Property &prop, const clang::FieldDecl &method, const std::string &methodName);

    bool typesMatch(const std::string &type1, clang::QualType type2Qt, std::string &type2Cleaned) const;
    std::unordered_map<std::string, clang::QualType> m_typedefMap;
};

#endif
