/*
    This file is part of the clazy static checker.

    Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
    Author: Sérgio Martins <sergio.martins@kdab.com>

    Copyright (C) 2015,2018 Sergio Martins <smartins@kde.org>

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

#include "function-args-by-ref.h"
#include "Utils.h"
#include "TypeUtils.h"
#include "ClazyContext.h"
#include "StringUtils.h"
#include "SourceCompatibilityHelpers.h"
#include "clazy_stl.h"
#include "FixItUtils.h"

#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/Type.h>
#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/LLVM.h>
#include <llvm/ADT/ArrayRef.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Casting.h>

#include <vector>

using namespace clang;
using namespace std;

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
    return clazy::contains(ignoreList, record->getQualifiedNameAsString());
}

static bool shouldIgnoreOperator(FunctionDecl *function)
{
    // Too many warnings in operator<<
    static const vector<StringRef> ignoreList = { "operator<<" };

    return clazy::contains(ignoreList, clazy::name(function));
}

static bool shouldIgnoreFunction(clang::FunctionDecl *function)
{
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

    return clazy::contains(qualifiedIgnoreList, function->getQualifiedNameAsString());
}

FunctionArgsByRef::FunctionArgsByRef(const std::string &name, ClazyContext *context)
    : CheckBase(name, context, Option_CanIgnoreIncludes)
{
}

static std::string warningMsgForSmallType(int sizeOf, const std::string &typeName)
{
    std::string sizeStr = std::to_string(sizeOf);
    return "Missing reference on large type (sizeof " + typeName + " is " + sizeStr + " bytes)";
}

void FunctionArgsByRef::processFunction(FunctionDecl *func)
{
    if (!func || !func->isThisDeclarationADefinition() || func->isDeleted() || shouldIgnoreOperator(func))
        return;

    if (m_context->isQtDeveloper() && shouldIgnoreFunction(func))
        return;

    Stmt *body = func->getBody();

    auto funcParams = Utils::functionParameters(func);
    for (unsigned int i = 0; i < funcParams.size(); ++i) {
        ParmVarDecl* param = funcParams[i];
        const QualType paramQt = clazy::unrefQualType(param->getType());
        const Type *paramType = paramQt.getTypePtrOrNull();
        if (!paramType || paramType->isIncompleteType() || paramType->isDependentType())
            continue;

        if (shouldIgnoreClass(paramType->getAsCXXRecordDecl()))
            continue;

        clazy::QualTypeClassification classif;
        bool success = clazy::classifyQualType(m_context, param->getType(), param, classif, body);
        if (!success)
            continue;

        vector<CXXCtorInitializer *> ctorInits = Utils::ctorInitializer(dyn_cast<CXXConstructorDecl>(func), param);
        if (Utils::ctorInitializerContainsMove(ctorInits))
            continue;

        if (classif.passBigTypeByConstRef || classif.passNonTriviallyCopyableByConstRef) {
            string error;
            std::vector<FixItHint> fixits;
            const string paramStr = param->getType().getAsString();
            if (classif.passBigTypeByConstRef) {
                error = warningMsgForSmallType(classif.size_of_T, paramStr);
            } else if (classif.passNonTriviallyCopyableByConstRef) {
                error = "Missing reference on non-trivial type (" + paramStr + ')';
            }

            addFixits(fixits, func, i);
            emitWarning(clazy::getLocStart(param), error.c_str(), fixits);
        }
    }
}

void FunctionArgsByRef::addFixits(std::vector<FixItHint> &fixits, FunctionDecl *func, unsigned int parmIndex)
{    
    for (auto funcRedecl : func->redecls()) {
        auto funcParams = Utils::functionParameters(funcRedecl);
        if (funcParams.size() <= parmIndex)
            return;

        ParmVarDecl *param = funcParams[parmIndex];
        QualType paramQt = clazy::unrefQualType(param->getType());

        const bool isConst = paramQt.isConstQualified();

        if (!isConst) {
            SourceLocation start = clazy::getLocStart(param);
            fixits.push_back(clazy::createInsertion(start, "const "));
        }

        SourceLocation end = param->getLocation();
        fixits.push_back(clazy::createInsertion(end, "&"));
    }
}

void FunctionArgsByRef::VisitDecl(Decl *decl)
{
    processFunction(dyn_cast<FunctionDecl>(decl));
}

void FunctionArgsByRef::VisitStmt(Stmt *stmt)
{
    if (auto lambda = dyn_cast<LambdaExpr>(stmt)) {
        if (!shouldIgnoreFile(clazy::getLocStart(stmt)))
            processFunction(lambda->getCallOperator());
    }
}

clang::FixItHint FunctionArgsByRef::fixit(const ParmVarDecl *, clazy::QualTypeClassification)
{
    FixItHint fixit;
    return fixit;
}
