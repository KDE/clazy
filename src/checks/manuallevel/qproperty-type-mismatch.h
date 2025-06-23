/*
    SPDX-FileCopyrightText: 2019 Jean-Michaël Celerier <jeanmichael.celerier@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLAZY_QPROPERTY_TYPE_MISMATCH_H
#define CLAZY_QPROPERTY_TYPE_MISMATCH_H

#include "checkbase.h"

#include <clang/Basic/SourceLocation.h>

#include <string>
#include <unordered_map>
#include <vector>

namespace clang
{
class CXXMethodDecl;
class FieldDecl;
class MacroInfo;
class Token;
class TypeAliasDecl;
} // namespace clang

/**
 * See README-qproperty-type-mismatch.md for more info.
 */
class QPropertyTypeMismatch : public CheckBase
{
public:
    explicit QPropertyTypeMismatch(const std::string &name);
    void VisitDecl(clang::Decl *) override;

private:
    void VisitMethod(const clang::CXXMethodDecl &);
    void VisitField(const clang::FieldDecl &);
    void VisitTypedef(const clang::TypedefNameDecl *);

    void VisitMacroExpands(const clang::Token &MacroNameTok, const clang::SourceRange &range, const clang::MacroInfo *minfo = nullptr) override;

    struct Property {
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
