/*
    SPDX-FileCopyrightText: 2015 Klarälvdalens Datakonsult AB a KDAB Group company info@kdab.com
    SPDX-FileContributor: Sérgio Martins <sergio.martins@kdab.com>

    SPDX-FileCopyrightText: 2015 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "foreach.h"
#include "ClazyContext.h"
#include "HierarchyUtils.h"
#include "PreProcessorVisitor.h"
#include "QtUtils.h"
#include "SourceCompatibilityHelpers.h"
#include "StringUtils.h"
#include "TypeUtils.h"
#include "Utils.h"
#include "clazy_stl.h"

#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/Type.h>
#include <clang/Basic/LLVM.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Casting.h>

#include <unordered_map>
#include <vector>

namespace clang
{
class Decl;
class DeclContext;
} // namespace clang

using namespace clang;

Foreach::Foreach(const std::string &name, ClazyContext *context)
    : CheckBase(name, context, Option_CanIgnoreIncludes)
{
    context->enablePreprocessorVisitor();
}

void Foreach::VisitStmt(clang::Stmt *stmt)
{
    PreProcessorVisitor *preProcessorVisitor = m_context->preprocessorVisitor;
    if (!preProcessorVisitor || preProcessorVisitor->qtVersion() >= 50900) {
        // Disabled since 5.9 because the Q_FOREACH internals changed.
        // Not worth fixing it because range-loop is recommended
        return;
    }

    auto *forStm = dyn_cast<ForStmt>(stmt);
    if (forStm) {
        m_lastForStmt = forStm;
        return;
    }

    if (!m_lastForStmt) {
        return;
    }

    auto *constructExpr = dyn_cast<CXXConstructExpr>(stmt);
    if (!constructExpr || constructExpr->getNumArgs() < 1) {
        return;
    }

    CXXConstructorDecl *constructorDecl = constructExpr->getConstructor();
    if (!constructorDecl || clazy::name(constructorDecl) != "QForeachContainer") {
        return;
    }

    std::vector<DeclRefExpr *> declRefExprs;
    clazy::getChilds<DeclRefExpr>(constructExpr, declRefExprs);
    if (declRefExprs.empty()) {
        return;
    }

    // Get the container value declaration
    DeclRefExpr *declRefExpr = declRefExprs.front();
    auto *valueDecl = dyn_cast<ValueDecl>(declRefExpr->getDecl());
    if (!valueDecl) {
        return;
    }

    QualType containerQualType = constructExpr->getArg(0)->getType();
    const Type *containerType = containerQualType.getTypePtrOrNull();
    CXXRecordDecl *const containerRecord = containerType ? containerType->getAsCXXRecordDecl() : nullptr;

    if (!containerRecord) {
        return;
    }

    auto *rootBaseClass = Utils::rootBaseClass(containerRecord);
    StringRef containerClassName = clazy::name(rootBaseClass);
    const bool isQtContainer = clazy::isQtIterableClass(containerClassName);
    if (containerClassName.empty()) {
        emitWarning(clazy::getLocStart(stmt), "internal error, couldn't get class name of foreach container, please report a bug");
        return;
    }
    if (!isQtContainer) {
        emitWarning(clazy::getLocStart(stmt), "foreach with STL container causes deep-copy (" + rootBaseClass->getQualifiedNameAsString() + ')');
        return;
    } else if (containerClassName == "QVarLengthArray") {
        emitWarning(clazy::getLocStart(stmt), "foreach with QVarLengthArray causes deep-copy");
        return;
    }

    checkBigTypeMissingRef();

    if (isa<MaterializeTemporaryExpr>(constructExpr->getArg(0))) { // Nothing else to check
        return;
    }

    // const containers are fine
    if (valueDecl->getType().isConstQualified()) {
        return;
    }

    // Now look inside the for statement for detachments
    if (containsDetachments(m_lastForStmt, valueDecl)) {
        emitWarning(clazy::getLocStart(stmt), "foreach container detached");
    }
}

void Foreach::checkBigTypeMissingRef()
{
    // Get the inner forstm
    std::vector<ForStmt *> forStatements;
    clazy::getChilds<ForStmt>(m_lastForStmt->getBody(), forStatements);
    if (forStatements.empty()) {
        return;
    }

    // Get the variable declaration (lhs of foreach)
    std::vector<DeclStmt *> varDecls;
    clazy::getChilds<DeclStmt>(forStatements.at(0), varDecls);
    if (varDecls.empty()) {
        return;
    }

    Decl *decl = varDecls.at(0)->getSingleDecl();
    VarDecl *varDecl = decl ? dyn_cast<VarDecl>(decl) : nullptr;
    if (!varDecl) {
        return;
    }

    clazy::QualTypeClassification classif;
    bool success = clazy::classifyQualType(m_context, varDecl->getType(), varDecl, /*by-ref*/ classif, forStatements.at(0));
    if (!success) {
        return;
    }

    if (classif.passBigTypeByConstRef || classif.passNonTriviallyCopyableByConstRef || classif.passSmallTrivialByValue) {
        std::string error;
        const std::string paramStr = varDecl->getType().getAsString();
        if (classif.passBigTypeByConstRef) {
            error = "Missing reference in foreach with sizeof(T) = ";
            error += std::to_string(classif.size_of_T) + " bytes (" + paramStr + ')';
        } else if (classif.passNonTriviallyCopyableByConstRef) {
            error = "Missing reference in foreach with non trivial type (" + paramStr + ')';
        } else if (classif.passSmallTrivialByValue) {
            // error = "Pass small and trivially-copyable type by value (" + paramStr + ')';
            //  Don't warn. The compiler can (and most do) optimize this and generate the same code
            return;
        }

        emitWarning(clazy::getLocStart(varDecl), error);
    }
}

bool Foreach::containsDetachments(Stmt *stm, clang::ValueDecl *containerValueDecl)
{
    if (!stm) {
        return false;
    }

    auto *memberExpr = dyn_cast<MemberExpr>(stm);
    if (memberExpr) {
        ValueDecl *valDecl = memberExpr->getMemberDecl();
        if (valDecl && valDecl->isCXXClassMember()) {
            DeclContext *declContext = valDecl->getDeclContext();
            auto *recordDecl = dyn_cast<CXXRecordDecl>(declContext);
            if (recordDecl) {
                const std::string className = Utils::rootBaseClass(recordDecl)->getQualifiedNameAsString();
                const std::unordered_map<std::string, std::vector<StringRef>> &detachingMethodsMap = clazy::detachingMethods();
                if (detachingMethodsMap.find(className) != detachingMethodsMap.end()) {
                    const std::string functionName = valDecl->getNameAsString();
                    const auto &allowedFunctions = detachingMethodsMap.at(className);
                    if (clazy::contains(allowedFunctions, functionName)) {
                        Expr *expr = memberExpr->getBase();

                        if (expr) {
                            auto *refExpr = dyn_cast<DeclRefExpr>(expr);
                            if (!refExpr) {
                                auto *s = clazy::getFirstChildAtDepth(expr, 1);
                                refExpr = dyn_cast<DeclRefExpr>(s);
                                if (refExpr) {
                                    if (refExpr->getDecl()
                                        == containerValueDecl) { // Finally, check if this non-const member call is on the same container we're iterating
                                        return true;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    return clazy::any_of(stm->children(), [this, containerValueDecl](Stmt *child) {
        return this->containsDetachments(child, containerValueDecl);
    });
}
