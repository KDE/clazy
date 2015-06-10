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

#include "Utils.h"

#include <clang/AST/DeclCXX.h>
#include <clang/AST/DeclTemplate.h>
#include <clang/AST/ExprCXX.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/AST/ParentMap.h>
#include <string>

using namespace clang;
using namespace std;

bool Utils::isQObject(CXXRecordDecl *decl)
{
    if (!decl)
        return false;

    if (decl->getName() == "QObject")
        return true;

    for (CXXRecordDecl::base_class_iterator it = decl->bases_begin();
         it != decl->bases_end();  ++it) {

        CXXBaseSpecifier *base = it;
        const Type *type = base->getType().getTypePtr();
        CXXRecordDecl *baseDecl = type->getAsCXXRecordDecl();
        if (isQObject(baseDecl)) {
            return true;
        }
    }

    return false;
}

void Utils::emitWarning(CompilerInstance &instance, SourceLocation loc, const char *error)
{
    FullSourceLoc full(loc, instance.getSourceManager());
    unsigned id = instance.getDiagnostics().getDiagnosticIDs()->getCustomDiagID(DiagnosticIDs::Warning, error);
    DiagnosticBuilder B = instance.getDiagnostics().Report(full, id);
}

bool Utils::hasEnding(const std::string &fullString, const std::string &ending)
{
    if (fullString.length() >= ending.length()) {
        return (0 == fullString.compare(fullString.length() - ending.length(),
                                        ending.length(), ending));
    }

    return false;
}

bool Utils::isChildOf(CXXRecordDecl *childDecl, CXXRecordDecl *parentDecl)
{
    if (!childDecl || !parentDecl)
        return false;

    if (childDecl == parentDecl)
        return false;

    for (CXXRecordDecl::base_class_iterator it = childDecl->bases_begin();
         it != childDecl->bases_end();  ++it) {

        CXXBaseSpecifier *base = it;
        const Type *type = base->getType().getTypePtrOrNull();
        if (!type) continue;
        CXXRecordDecl *baseDecl = type->getAsCXXRecordDecl();

        if (parentDecl == baseDecl) {
            return true;
        }

        if (isChildOf(baseDecl, parentDecl)) {
            return true;
        }
    }

    return false;
}

bool Utils::hasConstexprCtor(CXXRecordDecl *decl)
{
    for (CXXRecordDecl::ctor_iterator it = decl->ctor_begin(); it != decl->ctor_end(); ++it) {
        CXXConstructorDecl *ctor = *it;
        if (ctor->isConstexpr())
            return true;
    }
    return false;
}

ClassTemplateSpecializationDecl *Utils::templateDecl(Decl *decl)
{
    VarDecl *varDecl = dyn_cast<VarDecl>(decl);
    if (!varDecl) return 0;
    QualType qt = varDecl->getType();
    const Type *t = qt.getTypePtrOrNull();
    if (!t) return 0;
    CXXRecordDecl *classDecl = t->getAsCXXRecordDecl();
    if (!classDecl) return 0;
    return dyn_cast<ClassTemplateSpecializationDecl>(classDecl);
}

CXXRecordDecl * Utils::namedCastInnerDecl(CXXNamedCastExpr *staticOrDynamicCast)
{
    Expr *e = staticOrDynamicCast->getSubExpr();
    if (!e) return 0;
    QualType qt = e->getType();
    const Type *t = qt.getTypePtrOrNull();
    if (!t) return 0;
    QualType qt2 = t->getPointeeType();
    const Type *t2 = qt2.getTypePtrOrNull();
    if (!t2) return 0;
    return t2->getAsCXXRecordDecl();
}

CXXRecordDecl * Utils::namedCastOuterDecl(CXXNamedCastExpr *staticOrDynamicCast)
{
    QualType qt = staticOrDynamicCast->getTypeAsWritten();
    const Type *t = qt.getTypePtrOrNull();
    QualType qt2 = t->getPointeeType();
    const Type *t2 = qt2.getTypePtrOrNull();
    if (!t2) return 0;
    return t2->getAsCXXRecordDecl();
}

/*
void printLocation(const SourceLocation &start, const SourceLocation &end)
{
    SourceManager &sm = m_ci.getSourceManager();

    LangOptions lopt;
    clang::SourceLocation b(start), _e(end);
    clang::SourceLocation e(clang::Lexer::getLocForEndOfToken(_e, 0, sm, lopt));


    std::string resultText = " " + std::string(sm.getCharacterData(b), sm.getCharacterData(e)-sm.getCharacterData(b));
    std::string filename = sm.getFilename(start);
    int linenumber = sm.getSpellingLineNumber(start);

    llvm::errs() << filename << ":" << linenumber << resultText << "\n";
//        Utils::emitWarning(m_ci, start, "Use qobject_cast rather than dynamic_cast");
}
*/


/*
bool Utils::statementIsInFunc(clang::ParentMap *parentMap, clang::Stmt *stmt, const std::string &name)
{
    if (!stmt)
        return false;

    CXXMethodDecl *method = dyn_cast<Stmt>(stmt);
    if (!method)
        return statementIsInFunc(parentMap, parentMap->getParent(stmt), name);


    llvm::errs() << "Non-const method is: " << methodDecl->getQualifiedNameAsString() << "\n";

    return true;
}
*/


bool Utils::isParentOfMemberFunctionCall(Stmt *stm, const std::string &name)
{
    if (!stm)
        return false;

    auto expr = dyn_cast<MemberExpr>(stm);

    if (expr) {
        auto namedDecl = dyn_cast<NamedDecl>(expr->getMemberDecl());
        if (namedDecl && namedDecl->getNameAsString() == name)
            return true;
    }

    auto it = stm->child_begin();
    auto e = stm->child_end();
    for (; it != e; ++it) {
        if (isParentOfMemberFunctionCall(*it, name))
            return true;
    }

    return false;
}


bool Utils::allChildrenMemberCallsConst(Stmt *stm)
{
    if (!stm)
        return false;

    auto expr = dyn_cast<MemberExpr>(stm);

    if (expr) {
        auto methodDecl = dyn_cast<CXXMethodDecl>(expr->getMemberDecl());
        if (methodDecl && !methodDecl->isConst())
            return false;
    }

    auto it = stm->child_begin();
    auto e = stm->child_end();
    for (; it != e; ++it) {
        if (!allChildrenMemberCallsConst(*it))
            return false;
    }

    return true;
}

bool Utils::childsHaveSideEffects(Stmt *stm)
{
    if (!stm)
        return false;

    auto unary = dyn_cast<UnaryOperator>(stm);
    if (unary && (unary->isIncrementOp() || unary->isDecrementOp()))
        return true;

    auto binary = dyn_cast<BinaryOperator>(stm);
    if (binary && (binary->isAssignmentOp() || binary->isShiftAssignOp() || binary->isCompoundAssignmentOp()))
        return true;

    static std::vector<std::string> method_blacklist;
    if (method_blacklist.empty()) {
        method_blacklist.push_back("isDestroyed");
        method_blacklist.push_back("isRecursive"); // TODO: Use qualified name instead ?
        method_blacklist.push_back("q_func");
        method_blacklist.push_back("d_func");
        method_blacklist.push_back("begin");
        method_blacklist.push_back("end");
        method_blacklist.push_back("data");
        method_blacklist.push_back("fragment");
        method_blacklist.push_back("glIsRenderbuffer");
    }

    auto memberCall = dyn_cast<MemberExpr>(stm);
    if (memberCall) {
        auto methodDecl = dyn_cast<CXXMethodDecl>(memberCall->getMemberDecl());
        if (methodDecl && !methodDecl->isConst() && !methodDecl->isStatic() &&
                std::find(method_blacklist.cbegin(), method_blacklist.cend(), methodDecl->getNameAsString()) == method_blacklist.cend())
            return true;
    }

    /* // too many false positives, qIsFinite() etc for example
    auto callExpr = dyn_cast<CallExpr>(stm);
    if (callExpr) {
        FunctionDecl *callee = callExpr->getDirectCallee();
        if (callee && callee->isGlobal())
            return true;
    }*/

    auto it = stm->child_begin();
    auto e = stm->child_end();
    for (; it != e; ++it) {
        if (childsHaveSideEffects(*it))
            return true;
    }

    return false;
}

CXXRecordDecl *Utils::recordFromVarDecl(Decl *decl)
{
    auto varDecl = dyn_cast<VarDecl>(decl);
    if (varDecl == nullptr)
        return nullptr;

    QualType qt = varDecl->getType();
    const Type *t = qt.getTypePtrOrNull();
    if (t == nullptr)
        return nullptr;

    return t->getAsCXXRecordDecl();
}

ClassTemplateSpecializationDecl *Utils::templateSpecializationFromVarDecl(Decl *decl)
{
    auto record = recordFromVarDecl(decl);
    if (record)
        return dyn_cast<ClassTemplateSpecializationDecl>(record);

    return nullptr;
}

ValueDecl *Utils::valueDeclForMemberCall(CXXMemberCallExpr *memberCall)
{
    if (memberCall == nullptr)
        return nullptr;

    Expr *implicitObject = memberCall->getImplicitObjectArgument();
    if (implicitObject == nullptr)
        return nullptr;

    auto declRefExpr = dyn_cast<DeclRefExpr>(implicitObject);
    auto memberExpr =  dyn_cast<MemberExpr>(implicitObject);
    if (declRefExpr) {
        return declRefExpr->getDecl();
    } else if (memberExpr) {
        return memberExpr->getMemberDecl();
    }

    // Maybe there's an implicit cast in between..
    vector<MemberExpr*> memberExprs;
    vector<DeclRefExpr*> declRefs;
    getChilds<MemberExpr>(implicitObject, memberExprs);
    getChilds<DeclRefExpr>(implicitObject, declRefs);

    if (!memberExprs.empty()) {
        return memberExprs.at(0)->getMemberDecl();
    }

    if (!declRefs.empty()) {
        return declRefs.at(0)->getDecl();
    }

    return nullptr;
}


ValueDecl *Utils::valueDeclForOperatorCall(CXXOperatorCallExpr *operatorCall)
{
    if (operatorCall == nullptr)
        return nullptr;

    for (auto it = operatorCall->child_begin(); it != operatorCall->child_end(); ++it) {
        if (*it == nullptr) // Can happen
            continue;

        auto declRefExpr = dyn_cast<DeclRefExpr>(*it);
        auto memberExpr =  dyn_cast<MemberExpr>(*it);
        if (declRefExpr) {
            return declRefExpr->getDecl();
        } else if (memberExpr) {
            return memberExpr->getMemberDecl();
        }
    }

    return nullptr;
}

bool Utils::isValueDeclInFunctionContext(clang::ValueDecl *valueDecl)
{
    if (valueDecl == nullptr)
        return false;

    DeclContext *context = valueDecl->getDeclContext();
    return context != nullptr && dyn_cast<FunctionDecl>(context);
}

bool Utils::loopCanBeInterrupted(clang::Stmt *stmt, clang::CompilerInstance &ci, const clang::SourceLocation &onlyBeforeThisLoc)
{
    if (stmt == nullptr)
        return false;

    if (dyn_cast<ReturnStmt>(stmt) != nullptr || dyn_cast<BreakStmt>(stmt) != nullptr || dyn_cast<ContinueStmt>(stmt) != nullptr) {
        if (onlyBeforeThisLoc.isValid()) {
            FullSourceLoc sourceLoc(stmt->getLocStart(), ci.getSourceManager());
            FullSourceLoc otherSourceLoc(onlyBeforeThisLoc, ci.getSourceManager());
            if (sourceLoc.isBeforeInTranslationUnitThan(otherSourceLoc))
                return true;
        } else {
            return true;
        }
    }

    auto end = stmt->child_end();
    for (auto it = stmt->child_begin(); it != end; ++it) {
        if (loopCanBeInterrupted(*it, ci, onlyBeforeThisLoc))
            return true;
    }

    return false;
}


std::string Utils::qualifiedNameForDeclarationOfMemberExr(MemberExpr *memberExpr)
{
    if (memberExpr == nullptr)
        return {};

    auto valueDecl = memberExpr->getMemberDecl();
    if (valueDecl == nullptr)
        return {};

    return valueDecl->getQualifiedNameAsString();
}

bool Utils::descendsFrom(clang::CXXRecordDecl *derived, const std::string &parentName)
{
    if (derived == nullptr)
        return false;

    if (derived->getNameAsString() == parentName)
        return true;

    auto it = derived->bases_begin();
    auto end = derived->bases_end();

    for (; it != end; ++it) {
        QualType qt = (*it).getType();
        const Type *t = qt.getTypePtrOrNull();
        if (t == nullptr)
            continue;
        if (Utils::descendsFrom(t->getAsCXXRecordDecl(), parentName))
            return true;
    }

    return false;
}
