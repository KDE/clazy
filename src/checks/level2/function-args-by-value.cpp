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

#include "function-args-by-value.h"
#include "Utils.h"
#include "checkmanager.h"
#include "StringUtils.h"
#include "TypeUtils.h"
#include "FixItUtils.h"

#include <clang/AST/AST.h>
#include <clang/Lex/Lexer.h>

using namespace clang;
using namespace std;

enum Fixit {
    FixitNone = 0,
    FixitAll = 0x1 // More granularity isn't needed I guess
};

// TODO, go over all these
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
    return clazy_std::contains(ignoreList, record->getQualifiedNameAsString());
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
    if (clazy_std::contains(ignoreList, function->getNameAsString()))
        return true;

    return clazy_std::contains(qualifiedIgnoreList, function->getQualifiedNameAsString());
}

FunctionArgsByValue::FunctionArgsByValue(const std::string &name, ClazyContext *context)
    : CheckBase(name, context)
{
}

void FunctionArgsByValue::VisitDecl(Decl *decl)
{
    processFunction(dyn_cast<FunctionDecl>(decl));
}

void FunctionArgsByValue::VisitStmt(Stmt *stmt)
{
    if (LambdaExpr *lambda = dyn_cast<LambdaExpr>(stmt))
        processFunction(lambda->getCallOperator());
}

void FunctionArgsByValue::processFunction(FunctionDecl *func)
{
    if (!func || !func->isThisDeclarationADefinition() ||
        func->isDeleted() || shouldIgnoreFunction(func))
        return;

    auto ctor = dyn_cast<CXXConstructorDecl>(func);
    if (ctor) {
        if (ctor->isCopyConstructor())
            return; // copy-ctor must take by ref
    }

    Stmt *body = func->getBody();

    int i = -1;
    for (auto param : Utils::functionParameters(func)) {
        i++;
        QualType paramQt = TypeUtils::unrefQualType(param->getType());
        const Type *paramType = paramQt.getTypePtrOrNull();
        if (!paramType || paramType->isIncompleteType() || paramType->isDependentType())
            continue;

        if (shouldIgnoreClass(paramType->getAsCXXRecordDecl()))
            continue;

        TypeUtils::QualTypeClassification classif;
        bool success = TypeUtils::classifyQualType(&m_astContext, param, classif, body);
        if (!success)
            continue;

        if (classif.passSmallTrivialByValue) {
            if (ctor) { // Implements fix for Bug #379342
                vector<CXXCtorInitializer *> initializers = Utils::ctorInitializer(ctor, param);
                bool found_by_ref_member_init = false;
                for (auto initializer : initializers) {
                    if (!initializer->isMemberInitializer())
                        continue; // skip base class initializer
                    FieldDecl *field = initializer->getMember();
                    if (!field)
                        continue;

                    QualType type = field->getType();
                    if (type.isNull() || type->isReferenceType()) {
                        found_by_ref_member_init = true;
                        break;
                    }
                }

                if (found_by_ref_member_init)
                    continue;
            }

            std::vector<FixItHint> fixits;
            if (isFixitEnabled(FixitAll)) {
                for (auto redecl : func->redecls()) { // Fix in both header and .cpp
                    FunctionDecl *fdecl = dyn_cast<FunctionDecl>(redecl);
                    const ParmVarDecl *param = fdecl->getParamDecl(i);
                    fixits.push_back(fixit(fdecl, param, classif));
                }
            }

            const string paramStr = param->getType().getAsString();
            string error = "Pass small and trivially-copyable type by value (" + paramStr + ')';
            emitWarning(param->getLocStart(), error.c_str(), fixits);
        }
    }
}

FixItHint FunctionArgsByValue::fixit(FunctionDecl *func, const ParmVarDecl *param,
                                     TypeUtils::QualTypeClassification)
{
    QualType qt = TypeUtils::unrefQualType(param->getType());
    qt.removeLocalConst();
    const string typeName = qt.getAsString(PrintingPolicy(lo()));
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
        llvm::errs() << "Internal error could not apply fixit " << startLoc.printToString(sm())
                     << ';' << endLoc.printToString(sm()) << "\n";
        return {};
    }

    return FixItUtils::createReplacement({ startLoc, endLoc }, replacement);
}

REGISTER_CHECK("function-args-by-value", FunctionArgsByValue, CheckLevel2)
