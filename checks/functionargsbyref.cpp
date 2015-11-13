/*
   This file is part of the clang-lazy static checker.

  Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
  Author: Sérgio Martins <sergio.martins@kdab.com>

  Copyright (C) 2015 Sergio Martins <smartins@kde.org>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

  As a special exception, permission is given to link this program
  with any edition of Qt, and distribute the resulting executable,
  without including the source code for Qt in the source distribution.
*/

#include "functionargsbyref.h"
#include "Utils.h"
#include "checkmanager.h"

#include <clang/AST/AST.h>

using namespace clang;
using namespace std;

static bool shouldIgnoreClass(const std::string &qualifiedClassName)
{
    static const vector<string> ignoreList = {"QDebug", // Too many warnings
                                              "QGenericReturnArgument",
                                              "QColor", // TODO: Remove in Qt6
                                              "QStringRef", // TODO: Remove in Qt6
                                              "QList::const_iterator", // TODO: Remove in Qt6
                                              "QJsonArray::const_iterator", // TODO: Remove in Qt6
                                              "QList<QString>::const_iterator",  // TODO: Remove in Qt6
                                              "QtMetaTypePrivate::QSequentialIterableImpl",
                                              "QtMetaTypePrivate::QAssociativeIterableImpl"

                                             };
    return std::find(ignoreList.cbegin(), ignoreList.cend(), qualifiedClassName) != ignoreList.cend();
}

static bool shouldIgnoreFunction(const std::string &methodName)
{
    // Too many warnings in operator<<
    static const vector<string> ignoreList = {"operator<<"};
    return std::find(ignoreList.cbegin(), ignoreList.cend(), methodName) != ignoreList.cend();
}

FunctionArgsByRef::FunctionArgsByRef(const std::string &name)
    : CheckBase(name)
{
}

std::vector<string> FunctionArgsByRef::filesToIgnore() const
{
    return {"/c++/",
        "qimage.cpp", // TODO: Uncomment in Qt6
        "qimage.h",    // TODO: Uncomment in Qt6
        "qevent.h", // TODO: Uncomment in Qt6
        "avxintrin.h",
        "avx2intrin.h",
        "qnoncontiguousbytedevice.cpp",
        "qlocale_unix.cpp",
        "/clang/"
    };
}

static std::string warningMsgForSmallType(int sizeOf, const std::string &typeName)
{
    std::string sizeStr = std::to_string(sizeOf);
    return "Missing reference on large type sizeof " + typeName + " is " + sizeStr + " bytes)";
}

void FunctionArgsByRef::VisitDecl(Decl *decl)
{
    FunctionDecl *functionDecl = dyn_cast<FunctionDecl>(decl);
    if (!functionDecl || !functionDecl->hasBody() || shouldIgnoreFunction(functionDecl->getNameAsString())
            || !functionDecl->isThisDeclarationADefinition()) return;

    Stmt *body = functionDecl->getBody();

    for (auto it = functionDecl->param_begin(), end = functionDecl->param_end(); it != end; ++it) {
        const ParmVarDecl *param = *it;
        QualType paramQt = param->getType();
        const Type *paramType = paramQt.getTypePtrOrNull();
        if (!paramType || paramType->isDependentType())
            continue;

        CXXRecordDecl *recordDecl = paramType->getAsCXXRecordDecl();
        if (recordDecl && shouldIgnoreClass(recordDecl->getQualifiedNameAsString()))
            continue;

        Utils::QualTypeClassification classif;

        bool success = Utils::classifyQualType(param, classif, body);
        if (!success)
            continue;

        if (classif.passBigTypeByConstRef || classif.passNonTriviallyCopyableByConstRef) {
            string error;
            if (classif.passBigTypeByConstRef) {
                error = warningMsgForSmallType(classif.size_of_T, paramQt.getAsString());
            } else if (classif.passNonTriviallyCopyableByConstRef) {
                error = "Missing reference on non-trivial type " + recordDecl->getQualifiedNameAsString();
            }

            emitWarning(param->getLocStart(), error.c_str());
        }
    }
}

REGISTER_CHECK_WITH_FLAGS("function-args-by-ref", FunctionArgsByRef, CheckLevel2)
