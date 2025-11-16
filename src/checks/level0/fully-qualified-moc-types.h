/*
    SPDX-FileCopyrightText: 2018 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLAZY_FULLY_QUALIFIED_MOC_TYPES_H
#define CLAZY_FULLY_QUALIFIED_MOC_TYPES_H

#include "checkbase.h"

#include <string>
#include <vector>

namespace clang
{
class CXXMethodDecl;
class CXXRecordDecl;
} // namespace clang

/**
 * See README-fully-qualified-moc-types.md for more info.
 */
class FullyQualifiedMocTypes : public CheckBase
{
public:
    using CheckBase::CheckBase;
    void VisitDecl(clang::Decl *) override;

private:
    bool isGadget(clang::CXXRecordDecl *record) const;
    bool handleQ_PROPERTY(clang::CXXMethodDecl *);
    void VisitMacroExpands(const clang::Token &MacroNameTok, const clang::SourceRange &range, const clang::MacroInfo *minfo = nullptr) override;
    void registerQ_GADGET(clang::SourceLocation);
    bool typeIsFullyQualified(clang::QualType t, std::string &qualifiedTypeName, const std::string &typeName) const;
    std::string getQualifiedNameOfType(const clang::Type *ptr, bool resolveTemplateArgs) const;
    std::string resolveTemplateType(const clang::TemplateSpecializationType *ptr, bool resolveTemplateArgs) const;

    std::string writtenType(const clang::ParmVarDecl *param) const;

    std::vector<clang::SourceLocation> m_qgadgetMacroLocations;
};
#endif
