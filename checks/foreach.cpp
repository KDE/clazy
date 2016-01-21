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

#include "foreach.h"
#include "Utils.h"
#include "HierarchyUtils.h"
#include "QtUtils.h"
#include "checkmanager.h"

#include <clang/AST/AST.h>

using namespace clang;
using namespace std;

const std::map<std::string, std::vector<std::string> > & detachingMethodsMap()
{
    // List of methods causing detach
    static std::map<std::string, std::vector<std::string> > methodsMap;
    if (methodsMap.empty()) {
        methodsMap["QListSpecialMethods"] = {"first", "last", "begin", "end", "front", "back"};
        methodsMap["QList"] = methodsMap["QListSpecialMethods"];
        methodsMap["QVarLengthArray"] = methodsMap["QListSpecialMethods"];
        methodsMap["QVector"] = {"first", "last", "begin", "end", "front", "back", "data", "fill"};
        methodsMap["QMap"] = {"begin", "end", "first", "find", "last", "lowerBound", "upperBound"};
        methodsMap["QHash"] = {"begin", "end", "find"};
        methodsMap["QLinkedList"] = {"first", "last", "begin", "end", "front", "back"};
        methodsMap["QSet"] = {"begin", "end", "find"};
        methodsMap["QStack"] = methodsMap["QVector"];
        methodsMap["QStack"].push_back({"top"});
        methodsMap["QQueue"] = methodsMap["QVector"];
        methodsMap["QQueue"].push_back({"head"});
        methodsMap["QString"] = {"begin", "end", "data"};
        methodsMap["QByteArray"] = {"data"};
        methodsMap["QImage"] = {"bits", "scanLine"};
    }

    return methodsMap;
}

Foreach::Foreach(const std::string &name, const clang::CompilerInstance &ci)
    : CheckBase(name, ci)
{
}


void Foreach::VisitStmt(clang::Stmt *stmt)
{
    auto forStm = dyn_cast<ForStmt>(stmt);
    if (forStm) {
        m_lastForStmt = forStm;
        return;
    }

    if (!m_lastForStmt)
        return;

    CXXConstructExpr *constructExpr = dyn_cast<CXXConstructExpr>(stmt);
    if (!constructExpr || constructExpr->getNumArgs() < 1)
        return;

    CXXConstructorDecl *constructorDecl = constructExpr->getConstructor();
    if (!constructorDecl || constructorDecl->getNameAsString() != "QForeachContainer")
        return;

    vector<DeclRefExpr*> declRefExprs;
    HierarchyUtils::getChilds2<DeclRefExpr>(constructExpr, declRefExprs);
    if (declRefExprs.empty())
        return;

    // Get the container value declaration
    DeclRefExpr *declRefExpr = declRefExprs.front();
    ValueDecl *valueDecl = dyn_cast<ValueDecl>(declRefExpr->getDecl());
    if (!valueDecl)
        return;


    QualType containerQualType = constructExpr->getArg(0)->getType();
    const Type *containerType = containerQualType.getTypePtrOrNull();
    CXXRecordDecl *const containerRecord = containerType ? containerType->getAsCXXRecordDecl() : nullptr;


    if (!containerRecord)
        return;

    auto rootBaseClass = Utils::rootBaseClass(containerRecord);
    const string containerClassName = rootBaseClass->getNameAsString();
    const bool isQtContainer = QtUtils::isQtIterableClass(containerClassName);
    if (containerClassName.empty()) {
        emitWarning(stmt->getLocStart(), "internal error, couldn't get class name of foreach container, please report a bug");
        return;
    } else {
        if (!isQtContainer) {
            emitWarning(stmt->getLocStart(), "foreach with STL container causes deep-copy (" + rootBaseClass->getQualifiedNameAsString() + ')');
            return;
        }
    }

    checkBigTypeMissingRef();

    if (isa<MaterializeTemporaryExpr>(constructExpr->getArg(0))) // Nothing else to check
        return;

    // const containers are fine
    if (valueDecl->getType().isConstQualified())
        return;

    // Now look inside the for statement for detachments
    if (containsDetachments(m_lastForStmt, valueDecl)) {
        emitWarning(stmt->getLocStart(), "foreach container detached");
    }
}

void Foreach::checkBigTypeMissingRef()
{
    // Get the inner forstm
    vector<ForStmt*> forStatements;
    HierarchyUtils::getChilds2<ForStmt>(m_lastForStmt->getBody(), forStatements);
    if (forStatements.empty())
        return;

    // Get the variable declaration (lhs of foreach)
    vector<DeclStmt*> varDecls;
    HierarchyUtils::getChilds2<DeclStmt>(forStatements.at(0), varDecls);
    if (varDecls.empty())
        return;

    Decl *decl = varDecls.at(0)->getSingleDecl();
    VarDecl *varDecl = decl ? dyn_cast<VarDecl>(decl) : nullptr;
    if (!varDecl)
        return;

    Utils::QualTypeClassification classif;
    bool success = Utils::classifyQualType(m_ci, varDecl, /*by-ref*/classif, forStatements.at(0));
    if (!success)
        return;

    if (classif.passBigTypeByConstRef || classif.passNonTriviallyCopyableByConstRef || classif.passSmallTrivialByValue) {
        string error;
        const string paramStr = varDecl->getType().getAsString();
        if (classif.passBigTypeByConstRef) {
            error = "Missing reference in foreach with sizeof(T) = ";
            error += std::to_string(classif.size_of_T) + " bytes (" + paramStr + ')';
        } else if (classif.passNonTriviallyCopyableByConstRef) {
            error = "Missing reference in foreach with non trivial type (" + paramStr + ')';
        } else if (classif.passSmallTrivialByValue) {
            error = "Pass small and trivially-copyable type by value (" + paramStr + ')';
        }

        emitWarning(varDecl->getLocStart(), error.c_str());
    }
}

bool Foreach::containsDetachments(Stmt *stm, clang::ValueDecl *containerValueDecl)
{
    if (!stm)
        return false;

    auto memberExpr = dyn_cast<MemberExpr>(stm);
    if (memberExpr) {
        ValueDecl *valDecl = memberExpr->getMemberDecl();
        if (valDecl && valDecl->isCXXClassMember()) {
            DeclContext *declContext = valDecl->getDeclContext();
            auto recordDecl = dyn_cast<CXXRecordDecl>(declContext);
            if (recordDecl) {
                const std::string className = Utils::rootBaseClass(recordDecl)->getQualifiedNameAsString();
                if (detachingMethodsMap().find(className) != detachingMethodsMap().end()) {
                    const std::string functionName = valDecl->getNameAsString();
                    const auto &allowedFunctions = detachingMethodsMap().at(className);
                    if (std::find(allowedFunctions.cbegin(), allowedFunctions.cend(), functionName) != allowedFunctions.cend()) {
                        Expr *expr = memberExpr->getBase();

                        if (expr) {
                            DeclRefExpr *refExpr = dyn_cast<DeclRefExpr>(expr);
                            if (!refExpr) {
                                auto s = HierarchyUtils::getFirstChildAtDepth(expr, 1);
                                refExpr = dyn_cast<DeclRefExpr>(s);
                                if (refExpr) {
                                    if (refExpr->getDecl() == containerValueDecl) { // Finally, check if this non-const member call is on the same container we're iterating
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

    for (auto it = stm->child_begin(), end = stm->child_end(); it != end; ++it) {
        if (containsDetachments(*it, containerValueDecl))
            return true;
    }

    return false;
}

REGISTER_CHECK_WITH_FLAGS("foreach", Foreach, CheckLevel1)
