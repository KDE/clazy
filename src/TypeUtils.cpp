/*
    This file is part of the clazy static checker.

    Copyright (C) 2016-2017 Sergio Martins <smartins@kde.org>

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

#include "TypeUtils.h"
#include "HierarchyUtils.h"
#include "StringUtils.h"
#include "Utils.h"
#include "StmtBodyRange.h"
#include "ClazyContext.h"

#include <clang/AST/ASTContext.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/Type.h>
#include <clang/Basic/LLVM.h>

using namespace clang;

bool clazy::classifyQualType(const ClazyContext *context, clang::QualType qualType,
                             const VarDecl *varDecl, QualTypeClassification &classif,
                             clang::Stmt *body)
{
    QualType unrefQualType = clazy::unrefQualType(qualType);
    const Type *paramType = unrefQualType.getTypePtrOrNull();
    if (!paramType || paramType->isIncompleteType())
        return false;

    if (isUndeducibleAuto(paramType))
        return false;

    classif.size_of_T = context->astContext.getTypeSize(unrefQualType) / 8;
    classif.isBig = classif.size_of_T > 16;
    CXXRecordDecl *recordDecl = paramType->getAsCXXRecordDecl();
    CXXMethodDecl *copyCtor = recordDecl ? Utils::copyCtor(recordDecl) : nullptr;
    classif.isNonTriviallyCopyable = recordDecl && (recordDecl->hasNonTrivialCopyConstructor() || recordDecl->hasNonTrivialDestructor() || (copyCtor && copyCtor->isDeleted()));
    classif.isReference = qualType->isLValueReferenceType();
    classif.isConst = unrefQualType.isConstQualified();

    if (qualType->isRValueReferenceType()) // && ref, nothing to do here
        return true;

    if (classif.isConst && !classif.isReference) {
        classif.passNonTriviallyCopyableByConstRef = classif.isNonTriviallyCopyable;
        if (classif.isBig) {
            classif.passBigTypeByConstRef = true;
        }
    } else if (classif.isConst && classif.isReference && !classif.isNonTriviallyCopyable && !classif.isBig) {
        classif.passSmallTrivialByValue = true;
    } else if (varDecl && !classif.isConst && !classif.isReference && (classif.isBig || classif.isNonTriviallyCopyable)) {
        if (body && (Utils::containsNonConstMemberCall(context->parentMap, body, varDecl) || Utils::isPassedToFunction(StmtBodyRange(body), varDecl, /*byrefonly=*/ true)))
            return true;

        classif.passNonTriviallyCopyableByConstRef = classif.isNonTriviallyCopyable;
        if (classif.isBig) {
            classif.passBigTypeByConstRef = true;
        }
    }

    return true;
}

bool clazy::isSmallTrivial(const ClazyContext *context, QualType qualType)
{
    if (qualType.isNull())
        return false;

    if (qualType->isPointerType())
        qualType = qualType->getPointeeType();

    if (qualType->isPointerType()) // We don't care about ** (We can change this whenever we have a use case)
        return false;

    QualType unrefQualType = clazy::unrefQualType(qualType);
    const Type *paramType = unrefQualType.getTypePtrOrNull();
    if (!paramType || paramType->isIncompleteType())
        return false;

    if (isUndeducibleAuto(paramType))
        return false;

    if (qualType->isRValueReferenceType()) // && ref, nothing to do here
        return false;

     CXXRecordDecl *recordDecl = paramType->getAsCXXRecordDecl();
     CXXMethodDecl *copyCtor = recordDecl ? Utils::copyCtor(recordDecl) : nullptr;
     const bool hasDeletedCopyCtor = copyCtor && copyCtor->isDeleted();
     const bool isTrivial = recordDecl && !recordDecl->hasNonTrivialCopyConstructor() && !recordDecl->hasNonTrivialDestructor() && !hasDeletedCopyCtor;

     if (isTrivial) {
         const auto typeSize = context->astContext.getTypeSize(unrefQualType) / 8;
         const bool isSmall = typeSize <= 16;
         return isSmall;
     }

     return false;
}

void clazy::heapOrStackAllocated(Expr *arg, const std::string &type,
                                     const clang::LangOptions &lo,
                                     bool &isStack, bool &isHeap)
{
    isStack = false;
    isHeap = false;
    if (isa<CXXNewExpr>(arg)) {
        isHeap = true;
        return;
    }

    std::vector<DeclRefExpr*> declrefs;
    clazy::getChilds(arg, declrefs, 3);

    std::vector<DeclRefExpr*> interestingDeclRefs;
    for (auto declref : declrefs) {
        auto t = declref->getType().getTypePtrOrNull();
        if (!t)
            continue;

        // Remove the '*' if it's a pointer
        QualType qt = t->isPointerType() ? t->getPointeeType()
                                         : declref->getType();

        if (t && type == clazy::simpleTypeName(qt, lo)) {
            interestingDeclRefs.push_back(declref);
        }
    }

    if (interestingDeclRefs.size() > 1) {
        // Too complex
        return;
    }

    if (!interestingDeclRefs.empty()) {
        auto declref = interestingDeclRefs[0];
        isStack = !declref->getType().getTypePtr()->isPointerType();
        isHeap = !isStack;
    }
}

bool clazy::derivesFrom(const CXXRecordDecl *derived, const CXXRecordDecl *possibleBase,
                            std::vector<CXXRecordDecl*> *baseClasses)
{
    if (!derived || !possibleBase || derived == possibleBase)
        return false;

    for (auto base : derived->bases()) {
        const Type *type = base.getType().getTypePtrOrNull();
        if (!type) continue;
        CXXRecordDecl *baseDecl = type->getAsCXXRecordDecl();
        baseDecl = baseDecl ? baseDecl->getCanonicalDecl() : nullptr;

        if (possibleBase == baseDecl || derivesFrom(baseDecl, possibleBase, baseClasses)) {
            if (baseClasses)
                baseClasses->push_back(baseDecl);
            return true;
        }
    }

    return false;
}

bool clazy::derivesFrom(const clang::CXXRecordDecl *derived, const std::string &possibleBase)
{
    if (!derived || !derived->hasDefinition())
        return false;

    if (derived->getQualifiedNameAsString() == possibleBase)
        return true;

    for (auto base : derived->bases()) {
        if (derivesFrom(recordFromBaseSpecifier(base), possibleBase))
            return true;
    }

    return false;
}

bool clazy::derivesFrom(QualType derivedQT, const std::string &possibleBase)
{
    derivedQT = pointeeQualType(derivedQT);
    const auto t = derivedQT.getTypePtrOrNull();
    return t ? derivesFrom(t->getAsCXXRecordDecl(), possibleBase) : false;
}
