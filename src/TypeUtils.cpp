/*
   This file is part of the clazy static checker.

  Copyright (C) 2016 Sergio Martins <smartins@kde.org>

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
#include "Utils.h"
#include "StmtBodyRange.h"
#include <HierarchyUtils.h>
#include <StringUtils.h>

#include <clang/Frontend/CompilerInstance.h>
#include <clang/AST/ASTContext.h>

using namespace clang;

int TypeUtils::sizeOfPointer(const clang::CompilerInstance &ci, clang::QualType qt)
{
    if (!qt.getTypePtrOrNull())
        return -1;
    // HACK: What's a better way of getting the size of a pointer ?
    auto &astContext = ci.getASTContext();
    return astContext.getTypeSize(astContext.getPointerType(qt));
}

bool TypeUtils::classifyQualType(const CompilerInstance &ci, const VarDecl *varDecl, QualTypeClassification &classif, clang::Stmt *body)
{
    if (!varDecl)
        return false;

    QualType qualType = TypeUtils::unrefQualType(varDecl->getType());
    const Type *paramType = qualType.getTypePtrOrNull();
    if (!paramType || paramType->isIncompleteType())
        return false;

    if (isUndeducibleAuto(paramType))
        return false;

    classif.size_of_T = ci.getASTContext().getTypeSize(qualType) / 8;
    classif.isBig = classif.size_of_T > 16;
    CXXRecordDecl *recordDecl = paramType->getAsCXXRecordDecl();
    classif.isNonTriviallyCopyable = recordDecl && (recordDecl->hasNonTrivialCopyConstructor() || recordDecl->hasNonTrivialDestructor());
    classif.isReference = varDecl->getType()->isLValueReferenceType();
    classif.isConst = qualType.isConstQualified();

    if (varDecl->getType()->isRValueReferenceType()) // && ref, nothing to do here
        return true;

    if (classif.isConst && !classif.isReference) {
        classif.passNonTriviallyCopyableByConstRef = classif.isNonTriviallyCopyable;
        if (classif.isBig) {
            classif.passBigTypeByConstRef = true;
        }
    } else if (classif.isConst && classif.isReference && !classif.isNonTriviallyCopyable && !classif.isBig) {
        classif.passSmallTrivialByValue = true;
    } else if (!classif.isConst && !classif.isReference && (classif.isBig || classif.isNonTriviallyCopyable)) {
        if (body && (Utils::containsNonConstMemberCall(body, varDecl) || Utils::isPassedToFunction(StmtBodyRange(body), varDecl, /*byrefonly=*/ true)))
            return true;
        classif.passNonTriviallyCopyableByConstRef = classif.isNonTriviallyCopyable;
        if (classif.isBig) {
            classif.passBigTypeByConstRef = true;
        }
    }

    return true;
}

void TypeUtils::heapOrStackAllocated(Expr *arg, const std::string &type,
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
    HierarchyUtils::getChilds(arg, declrefs, 3);

    std::vector<DeclRefExpr*> interestingDeclRefs;
    for (auto declref : declrefs) {
        auto t = declref->getType().getTypePtrOrNull();
        if (!t)
            continue;

        // Remove the '*' if it's a pointer
        QualType qt = t->isPointerType() ? t->getPointeeType()
                                         : declref->getType();

        if (t && type == StringUtils::simpleTypeName(qt, lo)) {
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

bool TypeUtils::derivesFrom(CXXRecordDecl *derived, CXXRecordDecl *possibleBase)
{
    if (!derived || !possibleBase || derived == possibleBase)
        return false;

    for (auto base : derived->bases()) {
        const Type *type = base.getType().getTypePtrOrNull();
        if (!type) continue;
        CXXRecordDecl *baseDecl = type->getAsCXXRecordDecl();

        if (possibleBase == baseDecl || derivesFrom(baseDecl, possibleBase)) {
            return true;
        }
    }

    return false;
}

bool TypeUtils::derivesFrom(clang::CXXRecordDecl *derived, const std::string &possibleBase)
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

bool TypeUtils::derivesFrom(QualType derivedQT, const std::string &possibleBase)
{
    derivedQT = pointeeQualType(derivedQT);
    const auto t = derivedQT.getTypePtrOrNull();
    return t ? derivesFrom(t->getAsCXXRecordDecl(), possibleBase) : false;
}
