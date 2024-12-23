/*
    SPDX-FileCopyrightText: 2016-2018 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "function-args-by-value.h"
#include "ClazyContext.h"
#include "FixItUtils.h"
#include "StringUtils.h"
#include "TypeUtils.h"
#include "Utils.h"
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

using namespace clang;

// TODO, go over all these
bool FunctionArgsByValue::shouldIgnoreClass(CXXRecordDecl *record)
{
    if (!record) {
        return false;
    }

    if (Utils::isSharedPointer(record)) {
        return true;
    }

    static const std::vector<std::string> ignoreList = {
        "QDebug", // Too many warnings
        "QGenericReturnArgument",
        "QColor", // TODO: Remove in Qt6
        "QStringRef", // TODO: Remove in Qt6
        "QList::const_iterator", // TODO: Remove in Qt6
        "QJsonArray::const_iterator", // TODO: Remove in Qt6
        "QList<QString>::const_iterator", // TODO: Remove in Qt6
        "QtMetaTypePrivate::QSequentialIterableImpl",
        "QtMetaTypePrivate::QAssociativeIterableImpl",
        "QVariantComparisonHelper",
        "QHashDummyValue",
        "QCharRef",
        "QString::Null",
    };
    return clazy::contains(ignoreList, record->getQualifiedNameAsString());
}

bool FunctionArgsByValue::shouldIgnoreOperator(FunctionDecl *function)
{
    // Too many warnings in operator<<
    static const std::vector<StringRef> ignoreList = {"operator<<"};

    return clazy::contains(ignoreList, clazy::name(function));
}

bool FunctionArgsByValue::shouldIgnoreFunction(clang::FunctionDecl *function)
{
    static const std::vector<std::string> qualifiedIgnoreList = {
        "QDBusMessage::createErrorReply", // Fixed in Qt6
        "QMenu::exec", // Fixed in Qt6
        "QTextFrame::iterator", // Fixed in Qt6
        "QGraphicsWidget::addActions", // Fixed in Qt6
        "QListWidget::mimeData", // Fixed in Qt6
        "QTableWidget::mimeData", // Fixed in Qt6
        "QTreeWidget::mimeData", // Fixed in Qt6
        "QWidget::addActions", // Fixed in Qt6
        "QSslCertificate::verify", // Fixed in Qt6
        "QSslConfiguration::setAllowedNextProtocols", // Fixed in Qt6
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
    if (auto *lambda = dyn_cast<LambdaExpr>(stmt)) {
        processFunction(lambda->getCallOperator());
    }
}

void FunctionArgsByValue::processFunction(FunctionDecl *func)
{
    if (!func || !func->isThisDeclarationADefinition() || func->isDeleted()) {
        return;
    }

    if (func->isDefaulted()) {
        // The C++ compiler enforces refs or do being used for defaulted methods
        // https://invent.kde.org/sdk/clazy/-/issues/25
        return;
    }
    auto *ctor = dyn_cast<CXXConstructorDecl>(func);
    if (ctor && ctor->isCopyConstructor()) {
        return; // copy-ctor must take by ref
    }

    const bool warnForOverriddenMethods = isOptionSet("warn-for-overridden-methods");
    if (!warnForOverriddenMethods && Utils::methodOverrides(dyn_cast<CXXMethodDecl>(func))) {
        // When overriding you can't change the signature. You should fix the base classes first
        return;
    }

    if (shouldIgnoreOperator(func)) {
        return;
    }

    if (m_context->isQtDeveloper() && shouldIgnoreFunction(func)) {
        return;
    }

    Stmt *body = func->getBody();

    int i = -1;
    for (auto *param : Utils::functionParameters(func)) {
        i++;
        const QualType paramQt = clazy::unrefQualType(param->getType());
        const Type *paramType = paramQt.getTypePtrOrNull();
        if (!paramType || paramType->isIncompleteType() || paramType->isDependentType()) {
            continue;
        }

        if (shouldIgnoreClass(paramType->getAsCXXRecordDecl())) {
            continue;
        }

        clazy::QualTypeClassification classif;
        bool success = clazy::classifyQualType(m_context, param->getType(), param, classif, body);
        if (!success) {
            continue;
        }

        if (classif.passSmallTrivialByValue) {
            if (ctor) { // Implements fix for Bug #379342
                std::vector<CXXCtorInitializer *> initializers = Utils::ctorInitializer(ctor, param);
                bool found_by_ref_member_init = false;
                for (auto *initializer : initializers) {
                    if (!initializer->isMemberInitializer()) {
                        continue; // skip base class initializer
                    }
                    FieldDecl *field = initializer->getMember();
                    if (!field) {
                        continue;
                    }

                    QualType type = field->getType();
                    if (type.isNull() || type->isReferenceType()) {
                        found_by_ref_member_init = true;
                        break;
                    }
                }

                if (found_by_ref_member_init) {
                    continue;
                }
            }

            std::vector<FixItHint> fixits;
            auto *method = dyn_cast<CXXMethodDecl>(func);
            const bool isVirtualMethod = method && method->isVirtual();
            if (!isVirtualMethod || warnForOverriddenMethods) { // Don't try to fix virtual methods, as build can fail
                for (auto *redecl : func->redecls()) { // Fix in both header and .cpp
                    auto *fdecl = dyn_cast<FunctionDecl>(redecl);
                    const ParmVarDecl *param = fdecl->getParamDecl(i);
                    fixits.push_back(fixit(fdecl, param, classif));
                }
            }

            const std::string paramStr = param->getType().getAsString(lo());
            std::string error = "Pass small and trivially-copyable type by value (" + paramStr + ')';
            emitWarning(param->getBeginLoc(), error, fixits);
        }
    }
}

FixItHint FunctionArgsByValue::fixit(FunctionDecl *func, const ParmVarDecl *param, clazy::QualTypeClassification)
{
    QualType qt = clazy::unrefQualType(param->getType());
    qt.removeLocalConst();
    const std::string typeName = qt.getAsString(PrintingPolicy(lo()));
    std::string replacement = typeName + ' ' + std::string(clazy::name(param));
    SourceLocation startLoc = param->getBeginLoc();
    SourceLocation endLoc = param->getEndLoc();

    const int numRedeclarations = std::distance(func->redecls_begin(), func->redecls_end());
    const bool definitionIsAlsoDeclaration = numRedeclarations == 1;
    const bool isDeclarationButNotDefinition = !func->doesThisDeclarationHaveABody();

    if (param->hasDefaultArg() && (isDeclarationButNotDefinition || definitionIsAlsoDeclaration)) {
        endLoc = param->getDefaultArg()->getBeginLoc().getLocWithOffset(-1);
        replacement += " =";
    }

    if (!startLoc.isValid() || !endLoc.isValid()) {
        llvm::errs() << "Internal error could not apply fixit " << startLoc.printToString(sm()) << ';' << endLoc.printToString(sm()) << "\n";
        return {};
    }

    return clazy::createReplacement({startLoc, endLoc}, replacement);
}
