/*
    SPDX-FileCopyrightText: 2015 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "copyable-polymorphic.h"
#include "AccessSpecifierManager.h"
#include "ClazyContext.h"
#include "FixItUtils.h"
#include "SourceCompatibilityHelpers.h"
#include "StringUtils.h"
#include "Utils.h"

#include <clang/AST/DeclCXX.h>
#include <clang/Basic/LLVM.h>
#include <clang/Basic/Specifiers.h>
#include <llvm/Support/Casting.h>

class ClazyContext;
namespace clang
{
class Decl;
} // namespace clang

using namespace clang;

/// Returns whether the class has non-private copy-ctor or copy-assign
static bool hasPublicCopy(const CXXRecordDecl *record)
{
    CXXConstructorDecl *copyCtor = Utils::copyCtor(record);
    const bool hasCallableCopyCtor = copyCtor && !copyCtor->isDeleted() && copyCtor->getAccess() == clang::AS_public;
    if (!hasCallableCopyCtor) {
        CXXMethodDecl *copyAssign = Utils::copyAssign(record);
        const bool hasCallableCopyAssign = copyAssign && !copyAssign->isDeleted() && copyAssign->getAccess() == clang::AS_public;
        if (!hasCallableCopyAssign) {
            return false;
        }
    }

    return true;
}

/// Checks if there's any base class with public copy
static bool hasPublicCopyInAncestors(const CXXRecordDecl *record)
{
    if (!record) {
        return false;
    }

    for (auto base : record->bases()) {
        if (const Type *t = base.getType().getTypePtrOrNull()) {
            CXXRecordDecl *baseRecord = t->getAsCXXRecordDecl();
            if (hasPublicCopy(baseRecord)) {
                return true;
            }
            if (hasPublicCopyInAncestors(t->getAsCXXRecordDecl())) {
                return true;
            }
        }
    }

    return false;
}

CopyablePolymorphic::CopyablePolymorphic(const std::string &name, ClazyContext *context)
    : CheckBase(name, context)
{
    context->enableAccessSpecifierManager();
}

void CopyablePolymorphic::VisitDecl(clang::Decl *decl)
{
    auto *record = dyn_cast<CXXRecordDecl>(decl);
    if (!record || !record->hasDefinition() || record->getDefinition() != record || !record->isPolymorphic()) {
        return;
    }

    if (!hasPublicCopy(record)) {
        return;
    }

    if (record->isEffectivelyFinal() && !hasPublicCopyInAncestors(record)) {
        // If the derived class is final, and all the base classes copy-ctors are protected or private then it's ok
        return;
    }

    emitWarning(record->getBeginLoc(), "Polymorphic class " + record->getQualifiedNameAsString() + " is copyable. Potential slicing.", fixits(record));
}

std::vector<clang::FixItHint> CopyablePolymorphic::fixits(clang::CXXRecordDecl *record)
{
    std::vector<FixItHint> result;
    if (!m_context->accessSpecifierManager) {
        return {};
    }

    const StringRef className = clazy::name(record);

    // Insert Q_DISABLE_COPY(classname) in the private section if one exists,
    // otherwise at the end of the class declaration
    SourceLocation pos = m_context->accessSpecifierManager->firstLocationOfSection(clang::AccessSpecifier::AS_private, record);

    if (pos.isValid()) {
        pos = Lexer::findLocationAfterToken(pos, clang::tok::colon, sm(), lo(), false);
        result.push_back(clazy::createInsertion(pos, std::string("\n\tQ_DISABLE_COPY(") + className.data() + std::string(")")));
    } else {
        pos = record->getBraceRange().getEnd();
        result.push_back(clazy::createInsertion(pos, std::string("\tQ_DISABLE_COPY(") + className.data() + std::string(")\n")));
    }

    // If the class has a default constructor, then we need to readd it,
    // as the disabled copy constructor removes it.
    // Add it in the public section if one exists, otherwise add a
    // public section at the top of the class declaration.
    if (record->hasDefaultConstructor()) {
        pos = m_context->accessSpecifierManager->firstLocationOfSection(clang::AccessSpecifier::AS_public, record);
        if (pos.isInvalid()) {
            pos = record->getBraceRange().getBegin().getLocWithOffset(1);
            result.push_back(clazy::createInsertion(pos, std::string("\npublic:\n\t") + className.data() + std::string("() = default;")));
        } else {
            pos = Lexer::findLocationAfterToken(pos, clang::tok::colon, sm(), lo(), false);
            result.push_back(clazy::createInsertion(pos, std::string("\n\t") + className.data() + std::string("() = default;")));
        }
    }

    return result;
}
