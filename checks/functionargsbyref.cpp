/**********************************************************************
**  Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
**  Author: Sérgio Martins <sergio.martins@kdab.com>
**
** This file may be distributed and/or modified under the terms of the
** GNU Lesser General Public License version 2.1 and version 3 as published by the
** Free Software Foundation and appearing in the file LICENSE.LGPL.txt included.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**********************************************************************/

#include "functionargsbyref.h"
#include "Utils.h"

#include <clang/AST/AST.h>

using namespace clang;
using namespace std;

static bool shouldIgnoreClass(const std::string &qualifiedClassName)
{
    static const vector<string> ignoreList = {"QDebug", // Too many warnings
                                              "QGenericReturnArgument",
                                              "QColor", // TODO: Remove in Qt6
                                              "QStringRef", // TODO: Remove in Qt6
                                              "QList::const_iterator" // TODO: Remove in Qt6
                                             };
    return std::find(ignoreList.cbegin(), ignoreList.cend(), qualifiedClassName) != ignoreList.cend();
}

static bool shouldIgnoreFunction(const std::string &methodName)
{
    // Too many warnings in operator<<
    static const vector<string> ignoreList = {"operator<<"};
    return std::find(ignoreList.cbegin(), ignoreList.cend(), methodName) != ignoreList.cend();
}

FunctionArgsByRef::FunctionArgsByRef(clang::CompilerInstance &ci)
    : CheckBase(ci)
{
}

std::string FunctionArgsByRef::name() const
{
    return "function-args-by-ref";
}

std::vector<string> FunctionArgsByRef::filesToIgnore() const
{
    return {"/c++/",
        "qimage.cpp", // TODO: Uncomment in Qt6
        "qimage.h",    // TODO: Uncomment in Qt6
        "qevent.h", // TODO: Uncomment in Qt6
        "avxintrin.h", // Some clang internal
        "avx2intrin.h" // Some clang internal
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
    if (functionDecl == nullptr || !functionDecl->hasBody() || shouldIgnoreFunction(functionDecl->getNameAsString())
            || !functionDecl->isThisDeclarationADefinition()) return;

    Stmt *body = functionDecl->getBody();

    for (auto it = functionDecl->param_begin(), end = functionDecl->param_end(); it != end; ++it) {
        const ParmVarDecl *param = *it;
        QualType paramQt = param->getType();
        const Type *paramType = paramQt.getTypePtrOrNull();
        if (paramType == nullptr || paramType->isDependentType())
            continue;

        const int size_of_T = m_ci.getASTContext().getTypeSize(paramQt) / 8;
        const bool isSmall = size_of_T <= 16; // TODO: What about arm ?
        CXXRecordDecl *recordDecl = paramType->getAsCXXRecordDecl();
        const bool isUserNonTrivial = recordDecl && (recordDecl->hasUserDeclaredCopyConstructor() || recordDecl->hasUserDeclaredDestructor());
        const bool isReference = paramType->isLValueReferenceType();
        const bool isConst = paramQt.isConstQualified();
        if (recordDecl && shouldIgnoreClass(recordDecl->getQualifiedNameAsString()))
            continue;

        std::string error;
        if (isConst && !isReference) {
            if (!isSmall) {
                error += warningMsgForSmallType(size_of_T, paramQt.getAsString());
            } else if (isUserNonTrivial) {
                error += "Missing reference on non-trivial type";
            }
        } else if (isConst && isReference && !isUserNonTrivial && isSmall) {
            //error += "Don't use by-ref on small trivial type";
        } else if (!isConst && !isReference && (!isSmall || isUserNonTrivial)) {
            if (Utils::containsNonConstMemberCall(body, param) || Utils::containsCallByRef(body, param))
                continue;
            if (!isSmall) {
                error += warningMsgForSmallType(size_of_T, paramQt.getAsString());
            } else if (isUserNonTrivial) {
                error += "Missing reference on non-trivial type";
            }
        }

        if (!error.empty()) {
            error += " [-Wmore-warnings-function-args-by-ref]";
            emitWarning(param->getLocStart(), error.c_str());
        }
    }
}
