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
#include "FixItUtils.h"
#include "checkmanager.h"

#include <clang/AST/AST.h>
#include <clang/Lex/Lexer.h>

using namespace clang;
using namespace std;

enum Fixit {
    FixitNone = 0,
    FixitAll = 0x1 // More granularity isn't needed I guess
};

static bool shouldIgnoreClass(CXXRecordDecl *record)
{
    if (!record)
        return false;

    if (Utils::isSharedPointer(record))
        return true;

    static const vector<string> ignoreList = {"QDebug", // Too many warnings
                                              "QGenericReturnArgument",
                                              "QColor", // TODO: Remove in Qt6
                                              "QStringRef", // TODO: Remove in Qt6
                                              "QList::const_iterator", // TODO: Remove in Qt6
                                              "QJsonArray::const_iterator", // TODO: Remove in Qt6
                                              "QList<QString>::const_iterator",  // TODO: Remove in Qt6
                                              "QtMetaTypePrivate::QSequentialIterableImpl",
                                              "QtMetaTypePrivate::QAssociativeIterableImpl",
                                              "QVariantComparisonHelper",
                                              "QHashDummyValue", "QCharRef", "QString::Null"
                                             };
    return std::find(ignoreList.cbegin(), ignoreList.cend(), record->getQualifiedNameAsString()) != ignoreList.cend();
}

static bool shouldIgnoreFunction(clang::FunctionDecl *function)
{
    // Too many warnings in operator<<
    static const vector<string> ignoreList = {"operator<<"};
    static const vector<string> qualifiedIgnoreList = {"QDBusMessage::createErrorReply", // Fixed in Qt6
                                                       "QMenu::exec", // Fixed in Qt6
                                                       "QTextFrame::iterator", // Fixed in Qt6
                                                       "QGraphicsWidget::addActions", // Fixed in Qt6
                                                       "QListWidget::mimeData", // Fixed in Qt6
                                                       "QTableWidget::mimeData", // Fixed in Qt6
                                                       "QTreeWidget::mimeData", // Fixed in Qt6
                                                       "QWidget::addActions", // Fixed in Qt6
                                                       "QSslCertificate::verify", // Fixed in Qt6
                                                       "QSslConfiguration::setAllowedNextProtocols" // Fixed in Qt6
                                                      };
    if (std::find(ignoreList.cbegin(), ignoreList.cend(), function->getNameAsString()) != ignoreList.cend())
        return true;

    return std::find(qualifiedIgnoreList.cbegin(), qualifiedIgnoreList.cend(), function->getQualifiedNameAsString()) != qualifiedIgnoreList.cend();
}

FunctionArgsByRef::FunctionArgsByRef(const std::string &name, const clang::CompilerInstance &ci)
    : CheckBase(name, ci)
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
        "/clang/",
        "qmetatype.h", // TODO: fix in Qt
        "qbytearray.h" // TODO: fix in Qt
    };
}

static std::string warningMsgForSmallType(int sizeOf, const std::string &typeName)
{
    std::string sizeStr = std::to_string(sizeOf);
    return "Missing reference on large type sizeof " + typeName + " is " + sizeStr + " bytes)";
}

void FunctionArgsByRef::processFunction(FunctionDecl *functionDecl)
{
    if (!functionDecl || shouldIgnoreFunction(functionDecl)
            || !functionDecl->isThisDeclarationADefinition()) return;

    Stmt *body = functionDecl->getBody();

    int i = -1;
    for (auto it = functionDecl->param_begin(), end = functionDecl->param_end(); it != end; ++it) {
        i++;
        const ParmVarDecl *param = *it;
        QualType paramQt = Utils::unrefQualType(param->getType());
        const Type *paramType = paramQt.getTypePtrOrNull();
        if (!paramType || paramType->isIncompleteType() || paramType->isDependentType())
            continue;

        if (shouldIgnoreClass(paramType->getAsCXXRecordDecl()))
            continue;

        Utils::QualTypeClassification classif;
        bool success = Utils::classifyQualType(m_ci, param, classif, body);
        if (!success)
            continue;

        if (classif.passBigTypeByConstRef || classif.passNonTriviallyCopyableByConstRef || classif.passSmallTrivialByValue) {
            string error;
            std::vector<FixItHint> fixits;
            const string paramStr = param->getType().getAsString();
            if (classif.passBigTypeByConstRef) {
                error = warningMsgForSmallType(classif.size_of_T, paramStr);
            } else if (classif.passNonTriviallyCopyableByConstRef) {
                error = "Missing reference on non-trivial type (" + paramStr + ')';
            } else if (classif.passSmallTrivialByValue) {
                error = "Pass small and trivially-copyable type by value (" + paramStr + ')';
                if (isFixitEnabled(FixitAll)) {
                    for (auto it = functionDecl->redecls_begin(), end = functionDecl->redecls_end(); it != end; ++it) { // Fix in both header and .cpp
                        FunctionDecl *fdecl = dyn_cast<FunctionDecl>(*it);
                        const ParmVarDecl *param = fdecl->getParamDecl(i);
                        fixits.push_back(fixitByValue(fdecl, param, classif));
                    }
                }
            }

            emitWarning(param->getLocStart(), error.c_str(), fixits);
        }
    }
}

void FunctionArgsByRef::VisitDecl(Decl *decl)
{
    processFunction(dyn_cast<FunctionDecl>(decl));
}

void FunctionArgsByRef::VisitStmt(Stmt *stmt)
{
    if (LambdaExpr *lambda = dyn_cast<LambdaExpr>(stmt)) {
        processFunction(lambda->getCallOperator());
    }
}

FixItHint FunctionArgsByRef::fixitByValue(FunctionDecl *func, const ParmVarDecl *param, const Utils::QualTypeClassification &)
{
    QualType qt = Utils::unrefQualType(param->getType());
    qt.removeLocalConst();
    const string typeName = qt.getAsString(PrintingPolicy(m_ci.getLangOpts()));
    string replacement = typeName + ' ' + string(param->getName());
    SourceLocation startLoc = param->getLocStart();
    SourceLocation endLoc = param->getLocEnd();

    const int numRedeclarations = std::distance(func->redecls_begin(), func->redecls_end());
    const bool definitionIsAlsoDeclaration = numRedeclarations == 1;
    const bool isDeclarationButNotDefinition = !func->doesThisDeclarationHaveABody();

    if (param->hasDefaultArg() && (isDeclarationButNotDefinition || definitionIsAlsoDeclaration)) {
        endLoc = param->getDefaultArg()->getLocStart().getLocWithOffset(-1);
        replacement += " =";
    }

    if (!startLoc.isValid() || !endLoc.isValid()) {
        llvm::errs() << "Internal error could not apply fixit " << startLoc.printToString(m_ci.getSourceManager())
                     << ';' << endLoc.printToString(m_ci.getSourceManager()) << "\n";
        return {};
    }

    return FixItUtils::createReplacement({ startLoc, endLoc }, replacement);
}

clang::FixItHint FunctionArgsByRef::fixitByConstRef(const ParmVarDecl *, const Utils::QualTypeClassification &)
{
    FixItHint fixit;
    return fixit;
}

const char *const s_checkName = "function-args-by-ref";
REGISTER_CHECK_WITH_FLAGS(s_checkName, FunctionArgsByRef, CheckLevel2)
// REGISTER_FIXIT(FixitAll, "fix-func-args", s_checkName)
