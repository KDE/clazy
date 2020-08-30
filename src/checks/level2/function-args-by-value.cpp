/*
    This file is part of the clazy static checker.

    Copyright (C) 2016-2018 Sergio Martins <smartins@kde.org>

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
#include "StringUtils.h"
#include "TypeUtils.h"
#include "FixItUtils.h"
#include "ClazyContext.h"
#include "SourceCompatibilityHelpers.h"
#include "clazy_stl.h"

#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/PrettyPrinter.h>
#include <clang/AST/Redeclarable.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/Type.h>
#include <clang/Basic/LLVM.h>
#include <clang/Basic/SourceLocation.h>
#include <llvm/ADT/ArrayRef.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/raw_ostream.h>

#include <iterator>
#include <vector>

namespace clang {
class Decl;
}  // namespace clang

using namespace clang;
using namespace std;

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

FunctionArgsByValue::FunctionArgsByValue(const std::string &name, ClazyContext *context)
    : CheckBase(name, context, Option_CanIgnoreIncludes)
{
}

void FunctionArgsByValue::VisitDecl(Decl *decl)
{
    processFunction(dyn_cast<FunctionDecl>(decl));
}

void FunctionArgsByValue::VisitStmt(Stmt *stmt)
{
    if (auto lambda = dyn_cast<LambdaExpr>(stmt))
        processFunction(lambda->getCallOperator());
}

void FunctionArgsByValue::processFunction(FunctionDecl *func)
{
    if (!func || !func->isThisDeclarationADefinition() || func->isDeleted())
        return;

    auto ctor = dyn_cast<CXXConstructorDecl>(func);
    if (ctor && ctor->isCopyConstructor())
        return; // copy-ctor must take by ref

    const bool warnForOverriddenMethods = isOptionSet("warn-for-overridden-methods");
    if (!warnForOverriddenMethods && Utils::methodOverrides(dyn_cast<CXXMethodDecl>(func))) {
        // When overriding you can't change the signature. You should fix the base classes first
        return;
    }

    if (shouldIgnoreOperator(func))
        return;

    if (m_context->isQtDeveloper() && shouldIgnoreFunction(func))
        return;

    Stmt *body = func->getBody();

    int i = -1;
    for (auto param : Utils::functionParameters(func)) {
        i++;
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
            auto method = dyn_cast<CXXMethodDecl>(func);
            const bool isVirtualMethod = method && method->isVirtual();
            if (!isVirtualMethod || warnForOverriddenMethods) { // Don't try to fix virtual methods, as build can fail
                for (auto redecl : func->redecls()) { // Fix in both header and .cpp
                    auto fdecl = dyn_cast<FunctionDecl>(redecl);
                    const ParmVarDecl *param = fdecl->getParamDecl(i);
                    fixits.push_back(fixit(fdecl, param, classif));
                }
            }

            const string paramStr = param->getType().getAsString();
            string error = "Pass small and trivially-copyable type by value (" + paramStr + ')';
            emitWarning(clazy::getLocStart(param), error.c_str(), fixits);
        }
    }
}

FixItHint FunctionArgsByValue::fixit(FunctionDecl *func, const ParmVarDecl *param,
                                     clazy::QualTypeClassification)
{
    QualType qt = clazy::unrefQualType(param->getType());
    qt.removeLocalConst();
    const string typeName = qt.getAsString(PrintingPolicy(lo()));
    string replacement = typeName + ' ' + string(clazy::name(param));
    SourceLocation startLoc = clazy::getLocStart(param);
    SourceLocation endLoc = clazy::getLocEnd(param);

    const int numRedeclarations = std::distance(func->redecls_begin(), func->redecls_end());
    const bool definitionIsAlsoDeclaration = numRedeclarations == 1;
    const bool isDeclarationButNotDefinition = !func->doesThisDeclarationHaveABody();

    if (param->hasDefaultArg() && (isDeclarationButNotDefinition || definitionIsAlsoDeclaration)) {
        endLoc = clazy::getLocStart(param->getDefaultArg()).getLocWithOffset(-1);
        replacement += " =";
    }

    if (!startLoc.isValid() || !endLoc.isValid()) {
        llvm::errs() << "Internal error could not apply fixit " << startLoc.printToString(sm())
                     << ';' << endLoc.printToString(sm()) << "\n";
        return {};
    }

    return clazy::createReplacement({ startLoc, endLoc }, replacement);
}
