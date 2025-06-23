/*
    SPDX-FileCopyrightText: 2015 Klarälvdalens Datakonsult AB a KDAB Group company info@kdab.com
    SPDX-FileContributor: Sérgio Martins <sergio.martins@kdab.com>

    SPDX-FileCopyrightText: 2015-2016 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "missing-typeinfo.h"
#include "QtUtils.h"
#include "StringUtils.h"
#include "TemplateUtils.h"
#include "TypeUtils.h"

#include <clang/AST/DeclCXX.h>
#include <clang/AST/DeclTemplate.h>
#include <clang/AST/Type.h>
#include <clang/Basic/SourceManager.h>
#include <llvm/ADT/StringRef.h>

using namespace clang;

/**
 * Returns true if the call is on a java-style iterator class.
 * Returns if sizeof(T) > sizeof(void*), which would make QList<T> inefficient
 */
inline bool checkTooBigForQList(clang::QualType qt, const clang::ASTContext *context)
{
    return (int)context->getTypeSize(qt) <= clazy::sizeOfPointer(context, qt);
}

MissingTypeInfo::MissingTypeInfo(const std::string &name)
    : CheckBase(name)
{
}

void MissingTypeInfo::VisitDecl(clang::Decl *decl)
{
    ClassTemplateSpecializationDecl *tstdecl = clazy::templateDecl(decl);
    if (!tstdecl) {
        return;
    }

    const bool isQList = clazy::name(tstdecl) == "QList";
    const bool isQVector = isQList ? false : clazy::name(tstdecl) == "QVector";

    if (!isQList && !isQVector) {
        registerQTypeInfo(tstdecl);
        return;
    }

    QualType qt2 = clazy::getTemplateArgumentType(tstdecl, 0);
    const Type *t = qt2.getTypePtrOrNull();
    CXXRecordDecl *record = t ? t->getAsCXXRecordDecl() : nullptr;
    if (!record || !record->getDefinition() || typeHasClassification(qt2)) {
        return; // Don't crash if we only have a fwd decl
    }

    const bool isCopyable = qt2.isTriviallyCopyableType(*astContext());
    const bool isTooBigForQList = isQList && checkTooBigForQList(qt2, astContext());

    if ((isQVector || isTooBigForQList) && isCopyable) {
        if (sm().isInSystemHeader(record->getBeginLoc())) {
            return;
        }

        std::string typeName = static_cast<std::string>(clazy::name(record));
        if (typeName == "QPair") { // QPair doesn't use Q_DECLARE_TYPEINFO, but rather a explicit QTypeInfo.
            return;
        }

        emitWarning(decl, "Missing Q_DECLARE_TYPEINFO: " + typeName);
        if (!m_context->m_isClangTidy) { // Diagnostics are sorted and emitted in order based on location/message. Not helpful in this case
            emitWarning(record, "Type declared here:", false);
        }
    }
}

void MissingTypeInfo::registerQTypeInfo(ClassTemplateSpecializationDecl *decl)
{
    if (clazy::name(decl) == "QTypeInfo") {
        const std::string typeName = clazy::getTemplateArgumentTypeStr(decl, 0, lo(), /**recordOnly=*/true);
        if (!typeName.empty()) {
            m_typeInfos.insert(typeName);
        }
    }
}

bool MissingTypeInfo::typeHasClassification(QualType qt) const
{
    return m_typeInfos.find(clazy::simpleTypeName(qt, lo())) != m_typeInfos.end();
}
