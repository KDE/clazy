/*
   This file is part of the clazy static checker.

  Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
  Author: Sérgio Martins <sergio.martins@kdab.com>

  Copyright (C) 2015 Sergio Martins <smartins@kde.org>

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

#include "Utils.h"
#include "MethodSignatureUtils.h"
#include "StringUtils.h"
#include "HierarchyUtils.h"

#include <clang/AST/ASTContext.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/DeclTemplate.h>
#include <clang/AST/DeclFriend.h>
#include <clang/AST/ExprCXX.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/AST/ParentMap.h>
#include <clang/Lex/Lexer.h>

#include <sstream>

using namespace clang;
using namespace std;

bool Utils::derivesFrom(CXXRecordDecl *derived, CXXRecordDecl *possibleBase)
{
    if (!derived || !possibleBase || derived == possibleBase)
        return false;

    for (auto base : derived->bases()) {
        const Type *type = base.getType().getTypePtrOrNull();
        if (!type) continue;
        CXXRecordDecl *baseDecl = type->getAsCXXRecordDecl();

        if (possibleBase == baseDecl || derivesFrom(baseDecl, possibleBase)) {
            return true;
        }
    }

    return false;
}

bool Utils::hasConstexprCtor(CXXRecordDecl *decl)
{
    return clazy_std::any_of(decl->ctors(), [](CXXConstructorDecl *ctor) {
        return ctor->isConstexpr();
    });
}

CXXRecordDecl * Utils::namedCastInnerDecl(CXXNamedCastExpr *staticOrDynamicCast)
{
    Expr *e = staticOrDynamicCast->getSubExpr();
    if (!e) return nullptr;
    QualType qt = e->getType();
    const Type *t = qt.getTypePtrOrNull();
    if (!t) return nullptr;
    QualType qt2 = t->getPointeeType();
    const Type *t2 = qt2.getTypePtrOrNull();
    if (!t2) return nullptr;
    return t2->getAsCXXRecordDecl();
}

CXXRecordDecl * Utils::namedCastOuterDecl(CXXNamedCastExpr *staticOrDynamicCast)
{
    QualType qt = staticOrDynamicCast->getTypeAsWritten();
    const Type *t = qt.getTypePtrOrNull();
    QualType qt2 = t->getPointeeType();
    const Type *t2 = qt2.getTypePtrOrNull();
    if (!t2) return nullptr;
    return t2->getAsCXXRecordDecl();
}

/*
void printLocation(const SourceLocation &start, const SourceLocation &end)
{
    SourceManager &sm = m_ci.getSourceManager();

    LangOptions lopt;
    clang::SourceLocation b(start), _e(end);
    clang::SourceLocation e(clang::Lexer::getLocForEndOfToken(_e, 0, sm, lopt));


    std::string resultText = ' ' + std::string(sm.getCharacterData(b), sm.getCharacterData(e)-sm.getCharacterData(b));
    std::string filename = sm.getFilename(start);
    int linenumber = sm.getSpellingLineNumber(start);

    llvm::errs() << filename << ':' << linenumber << resultText << "\n";
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

    return clazy_std::all_of(stm->children(), [](Stmt *child) {
        return allChildrenMemberCallsConst(child);
    });
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
                !clazy_std::contains(method_blacklist, methodDecl->getNameAsString()))
            return true;
    }

    /* // too many false positives, qIsFinite() etc for example
    auto callExpr = dyn_cast<CallExpr>(stm);
    if (callExpr) {
        FunctionDecl *callee = callExpr->getDirectCallee();
        if (callee && callee->isGlobal())
            return true;
    }*/

    return clazy_std::any_of(stm->children(), [](Stmt *s) {
        return childsHaveSideEffects(s);
    });
}

CXXRecordDecl *Utils::recordFromVarDecl(Decl *decl)
{
    auto varDecl = dyn_cast<VarDecl>(decl);
    if (!varDecl)
        return nullptr;

    QualType qt = varDecl->getType();
    const Type *t = qt.getTypePtrOrNull();
    if (!t)
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
    if (!memberCall)
        return nullptr;

    Expr *implicitObject = memberCall->getImplicitObjectArgument();
    if (!implicitObject)
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
    HierarchyUtils::getChildsHACK<MemberExpr>(implicitObject, memberExprs);
    HierarchyUtils::getChildsHACK<DeclRefExpr>(implicitObject, declRefs);

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
    if (!operatorCall)
        return nullptr;

    for (auto child : operatorCall->children()) {
        if (!child) // Can happen
            continue;

        auto declRefExpr = dyn_cast<DeclRefExpr>(child);
        auto memberExpr =  dyn_cast<MemberExpr>(child);
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
    DeclContext *context = valueDecl ? valueDecl->getDeclContext() : nullptr;
    return context && isa<FunctionDecl>(context) && !isa<ParmVarDecl>(valueDecl);
}

bool Utils::loopCanBeInterrupted(clang::Stmt *stmt, const clang::CompilerInstance &ci, const clang::SourceLocation &onlyBeforeThisLoc)
{
    if (!stmt)
        return false;

    if (isa<ReturnStmt>(stmt) || isa<BreakStmt>(stmt) || isa<ContinueStmt>(stmt)) {
        if (onlyBeforeThisLoc.isValid()) {
            FullSourceLoc sourceLoc(stmt->getLocStart(), ci.getSourceManager());
            FullSourceLoc otherSourceLoc(onlyBeforeThisLoc, ci.getSourceManager());
            if (sourceLoc.isBeforeInTranslationUnitThan(otherSourceLoc))
                return true;
        } else {
            return true;
        }
    }

    return clazy_std::any_of(stmt->children(), [&ci, onlyBeforeThisLoc](Stmt *s) {
        return Utils::loopCanBeInterrupted(s, ci, onlyBeforeThisLoc);
    });
}

bool Utils::derivesFrom(clang::CXXRecordDecl *derived, const std::string &possibleBase)
{
    if (!derived)
        return false;

    if (derived->getNameAsString() == possibleBase)
        return true;

    for (auto base : derived->bases()) {
        const Type *t = base.getType().getTypePtrOrNull();
        if (t && derivesFrom(t->getAsCXXRecordDecl(), possibleBase))
            return true;
    }

    return false;
}

bool Utils::containsNonConstMemberCall(Stmt *body, const VarDecl *varDecl)
{
    std::vector<CXXMemberCallExpr*> memberCalls;
    HierarchyUtils::getChilds<CXXMemberCallExpr>(body, memberCalls);

    for (auto it = memberCalls.cbegin(), end = memberCalls.cend(); it != end; ++it) {
        CXXMemberCallExpr *memberCall = *it;
        CXXMethodDecl *methodDecl = memberCall->getMethodDecl();
        if (!methodDecl || methodDecl->isConst())
            continue;

        ValueDecl *valueDecl = Utils::valueDeclForMemberCall(*it);
        if (!valueDecl)
            continue;

        if (valueDecl == varDecl)
            return true;
    }

    // Check for operator calls:
    std::vector<CXXOperatorCallExpr*> operatorCalls;
    HierarchyUtils::getChilds<CXXOperatorCallExpr>(body, operatorCalls);
    for (auto it = operatorCalls.cbegin(), end = operatorCalls.cend(); it != end; ++it) {
        CXXOperatorCallExpr *operatorExpr = *it;
        FunctionDecl *fDecl = operatorExpr->getDirectCallee();
        if (!fDecl)
            continue;
        CXXMethodDecl *methodDecl = dyn_cast<CXXMethodDecl>(fDecl);
        if (methodDecl == nullptr || methodDecl->isConst())
            continue;

        ValueDecl *valueDecl = Utils::valueDeclForOperatorCall(*it);
        if (!valueDecl)
            continue;

        if (valueDecl == varDecl)
            return true;
    }

    return false;
}

template<class T>
static bool isArgOfFunc(T expr, FunctionDecl *fDecl, const VarDecl *varDecl, bool byRefOnly)
{
    unsigned int param = 0;
    for (auto arg : expr->arguments()) {
        DeclRefExpr *refExpr = dyn_cast<DeclRefExpr>(arg);
        if (!refExpr)  {
            if (arg->child_begin() != arg->child_end()) {
                refExpr = dyn_cast<DeclRefExpr>(*(arg->child_begin()));
                if (!refExpr)
                    continue;
            } else {
                continue;
            }
        }

        if (refExpr->getDecl() != varDecl) // It's our variable ?
            continue;

        if (!byRefOnly) {
            // We found it
            return true;
        }

        // It is, lets see if the callee takes our variable by const-ref
        if (param >= fDecl->param_size())
            continue;

        ParmVarDecl *paramDecl = fDecl->getParamDecl(param);
        if (!paramDecl)
            continue;

        QualType qt = paramDecl->getType();
        const Type *t = qt.getTypePtrOrNull();
        if (!t)
            continue;

        if ((t->isReferenceType() || t->isPointerType()) && !t->getPointeeType().isConstQualified())
            return true; // function receives non-const ref, so our foreach variable cant be const-ref

        ++param;
    }

    return false;
}

bool Utils::isPassedToFunction(Stmt *body, const VarDecl *varDecl, bool byRefOnly)
{
    if (!body)
        return false;

    std::vector<CallExpr*> callExprs;
    HierarchyUtils::getChilds<CallExpr>(body, callExprs);
    for (auto it = callExprs.cbegin(), end = callExprs.cend(); it != end; ++it) {
        CallExpr *callexpr = *it;
        FunctionDecl *fDecl = callexpr->getDirectCallee();
        if (!fDecl)
            continue;

        if (isArgOfFunc(callexpr, fDecl, varDecl, byRefOnly))
            return true;
    }

    std::vector<CXXConstructExpr*> constructExprs;
    HierarchyUtils::getChilds<CXXConstructExpr>(body, constructExprs);
    for (auto it = constructExprs.cbegin(), end = constructExprs.cend(); it != end; ++it) {
        CXXConstructExpr *constructExpr = *it;
        FunctionDecl *fDecl = constructExpr->getConstructor();

        if (isArgOfFunc(constructExpr, fDecl, varDecl, byRefOnly))
            return true;
    }

    return false;
}

bool Utils::isAssignedTo(Stmt *body, const VarDecl *varDecl)
{
    if (!body)
        return false;

    std::vector<CXXOperatorCallExpr*> operatorCalls;
    HierarchyUtils::getChilds<CXXOperatorCallExpr>(body, operatorCalls);
    for (auto it = operatorCalls.cbegin(), end = operatorCalls.cend(); it != end; ++it) {
        CXXOperatorCallExpr *operatorExpr = *it;
        FunctionDecl *fDecl = operatorExpr->getDirectCallee();
        if (!fDecl)
            continue;

        CXXMethodDecl *methodDecl = dyn_cast<CXXMethodDecl>(fDecl);
        if (methodDecl && methodDecl->isCopyAssignmentOperator()) {
            ValueDecl *valueDecl = Utils::valueDeclForOperatorCall(operatorExpr);
            if (valueDecl == varDecl)
                return true;
        }
    }

    return false;
}

bool Utils::callHasDefaultArguments(clang::CallExpr *expr)
{
    std::vector<clang::CXXDefaultArgExpr*> exprs;
    HierarchyUtils::getChilds<clang::CXXDefaultArgExpr>(expr, exprs, 1);
    return !exprs.empty();
}

bool Utils::containsStringLiteral(Stmt *stm, bool allowEmpty, int depth)
{
    if (!stm)
        return false;

    std::vector<StringLiteral*> stringLiterals;
    HierarchyUtils::getChilds<StringLiteral>(stm, stringLiterals, depth);

    if (allowEmpty)
        return !stringLiterals.empty();

    for (StringLiteral *sl : stringLiterals) {
        if (sl->getLength() > 0)
            return true;
    }

    return false;
}

bool Utils::ternaryOperatorIsOfStringLiteral(ConditionalOperator *ternary)
{
    bool skipFirst = true;
    for (auto child : ternary->children()) {
        if (skipFirst) {
            skipFirst = false;
            continue;
        }

        if (isa<StringLiteral>(child))
            continue;

        auto arrayToPointerDecay = dyn_cast<ImplicitCastExpr>(child);
        if (!arrayToPointerDecay || !isa<StringLiteral>(*(arrayToPointerDecay->child_begin())))
            return false;
    }

    return true;
}

bool Utils::isAssignOperator(CXXOperatorCallExpr *op, const std::string &className, const std::string &argumentType)
{
    if (!op)
        return false;

    FunctionDecl *functionDecl = op->getDirectCallee();
    if (!functionDecl)
        return false;

    CXXMethodDecl *methodDecl = dyn_cast<clang::CXXMethodDecl>(functionDecl);
    if (!className.empty() && !isOfClass(methodDecl, className))
        return false;

    if (functionDecl->getNameAsString() != "operator=")
        return false;

    if (!argumentType.empty() && !hasArgumentOfType(functionDecl, argumentType, 1)) {
        return false;
    }

    return true;
}


bool Utils::isImplicitCastTo(Stmt *s, const string &className)
{
    ImplicitCastExpr *expr = dyn_cast<ImplicitCastExpr>(s);
    if (!expr)
        return false;

    auto record = expr->getBestDynamicClassType();
    return record && record->getNameAsString() == className;
}


bool Utils::isInsideOperatorCall(ParentMap *map, Stmt *s, const std::vector<string> &anyOf)
{
    if (!s)
        return false;

    CXXOperatorCallExpr *oper = dyn_cast<CXXOperatorCallExpr>(s);
    if (oper) {
        auto func = oper->getDirectCallee();
        if (func) {
            if (anyOf.empty())
                return true;

            auto method = dyn_cast<CXXMethodDecl>(func);
            if (method) {
                auto record = method->getParent();
                if (record && clazy_std::contains(anyOf, record->getNameAsString()))
                    return true;
            }
        }
    }

    return isInsideOperatorCall(map, HierarchyUtils::parent(map, s), anyOf);
}


bool Utils::insideCTORCall(ParentMap *map, Stmt *s, const std::vector<string> &anyOf)
{
    if (!s)
        return false;

    CXXConstructExpr *expr = dyn_cast<CXXConstructExpr>(s);
    if (expr && expr->getConstructor() && clazy_std::contains(anyOf, expr->getConstructor()->getNameAsString())) {
        return true;
    }

    return insideCTORCall(map, HierarchyUtils::parent(map, s), anyOf);
}

bool Utils::presumedLocationsEqual(const clang::PresumedLoc &l1, const clang::PresumedLoc &l2)
{
    return l1.getColumn() == l2.getColumn() &&
           l1.getLine()   == l2.getLine()   &&
            string(l1.getFilename()) == string(l2.getFilename());
}

CXXRecordDecl *Utils::isMemberVariable(ValueDecl *decl)
{
    return decl ? dyn_cast<CXXRecordDecl>(decl->getDeclContext()) : nullptr;
}

Stmt *Utils::bodyFromLoop(Stmt *loop)
{
    if (!loop)
        return nullptr;

    if (auto forstm = dyn_cast<ForStmt>(loop)) {
        return forstm->getBody();
    }

    if (auto whilestm = dyn_cast<WhileStmt>(loop)) {
        return whilestm->getBody();
    }

    if (auto dostm = dyn_cast<DoStmt>(loop)) {
        return dostm->getBody();
    }

    return nullptr;
}

std::vector<CXXMethodDecl *> Utils::methodsFromString(const CXXRecordDecl *record, const string &methodName)
{
    if (!record)
        return {};

    vector<CXXMethodDecl *> methods;

    for (auto method : record->methods()) {
        if (method->getNameAsString() == methodName)
            methods.push_back(method);
    }

    // Also include the base classes
    for (auto base : record->bases()) {
        const Type *t = base.getType().getTypePtrOrNull();
        if (t) {
            auto baseMethods = methodsFromString(t->getAsCXXRecordDecl(), methodName);
            if (!baseMethods.empty())
                std::copy(baseMethods.begin(), baseMethods.end(), std::back_inserter(methods));
        }
    }

    return methods;
}

const CXXRecordDecl *Utils::recordForMemberCall(CXXMemberCallExpr *call, string &implicitCallee)
{
    implicitCallee = {};
    Expr *implicitArgument= call->getImplicitObjectArgument();
    if (!implicitArgument) {
        return nullptr;
    }

    Stmt *s = implicitArgument;
    while (s) {
        if (auto declRef = dyn_cast<DeclRefExpr>(s)) {
            if (declRef->getDecl()) {
                implicitCallee = declRef->getDecl()->getNameAsString();
                QualType qt = declRef->getDecl()->getType();
                return qt->getPointeeCXXRecordDecl();
            } else {
                return nullptr;
            }
        } else if (auto thisExpr = dyn_cast<CXXThisExpr>(s)) {
            implicitCallee = "this";
            return thisExpr->getType()->getPointeeCXXRecordDecl();
        } else if (auto memberExpr = dyn_cast<MemberExpr>(s)) {
            auto decl = memberExpr->getMemberDecl();
            if (decl) {
                implicitCallee = decl->getNameAsString();
                QualType qt = decl->getType();
                return qt->getPointeeCXXRecordDecl();
            } else {
                return nullptr;
            }
        }

        s = s->child_begin() == s->child_end() ? nullptr : *(s->child_begin());
    }

    return nullptr;
}

static string nameForContext(DeclContext *context)
{
    if (auto  *ns = dyn_cast<NamespaceDecl>(context)) {
        return ns->getNameAsString();
    } else if (auto rec = dyn_cast<CXXRecordDecl>(context)) {
        return rec->getNameAsString();
    } else if (auto *method = dyn_cast<CXXMethodDecl>(context)) {
        return method->getNameAsString();
    } else if (dyn_cast<TranslationUnitDecl>(context)){
        return {};
    } else {
        llvm::errs() << "Unhandled kind: " << context->getDeclKindName() << "\n";
    }

    return {};
}

string Utils::getMostNeededQualifiedName(const SourceManager &sourceManager, CXXMethodDecl *method, DeclContext *currentScope, SourceLocation usageLoc, bool honourUsingDirectives)
{
    if (!currentScope)
        return method->getQualifiedNameAsString();

    // All namespaces, classes, inner class qualifications
    auto methodContexts = contextsForDecl(method->getDeclContext());

    // Visible scopes in current scope
    auto visibleContexts = contextsForDecl(currentScope);

    // Collect using directives
    vector<UsingDirectiveDecl*> usings;
    if (honourUsingDirectives) {
        for (DeclContext *context : visibleContexts) {
            auto range = context->using_directives();
            for (auto it = range.begin(), end = range.end(); it != end; ++it) {
                usings.push_back(*it);
            }
        }
    }

    for (UsingDirectiveDecl *u : usings) {
        NamespaceDecl *ns = u->getNominatedNamespace();
        if (ns) {
            if (sourceManager.isBeforeInSLocAddrSpace(usageLoc, u->getLocStart()))
                continue;

            visibleContexts.push_back(ns->getOriginalNamespace());
        }
    }

    for (DeclContext *context : visibleContexts) {

        if (context != method->getParent()) { // Don't remove the most immediate
            auto it = clazy_std::find_if(methodContexts, [context](DeclContext *c) {
                    if (c == context)
                        return true;
                    auto ns1 = dyn_cast<NamespaceDecl>(c);
                    auto ns2 = dyn_cast<NamespaceDecl>(context);
                    return ns1 && ns2 && ns1->getQualifiedNameAsString() == ns2->getQualifiedNameAsString();

                });
            if (it != methodContexts.end()) {
                methodContexts.erase(it, it + 1);
            }
        }
    }

    string neededContexts;
    for (DeclContext *context : methodContexts) {
        neededContexts = nameForContext(context) + "::" + neededContexts;
    }

    const string result = neededContexts + method->getNameAsString();
    return result;
}

std::vector<DeclContext *> Utils::contextsForDecl(DeclContext *currentScope)
{
    std::vector<DeclContext *> decls;
    while (currentScope) {
        decls.push_back(currentScope);
        currentScope = currentScope->getParent();
    }

    return decls;
}

bool Utils::canTakeAddressOf(CXXMethodDecl *method, DeclContext *context, bool &isSpecialProtectedCase)
{
    isSpecialProtectedCase = false;
    if (!method || !method->getParent())
        return false;

    if (method->getAccess() == clang::AccessSpecifier::AS_public)
        return true;

    if (!context)
        return false;

    CXXRecordDecl *contextRecord = nullptr;

    do {
        contextRecord = dyn_cast<CXXRecordDecl>(context);
        context = context->getParent();
    } while (contextRecord == nullptr && context);

    if (!contextRecord) // If we're not inside a class method we can't take the address of a private/protected method
        return false;

    CXXRecordDecl *record = method->getParent();
    if (record == contextRecord)
        return true;

    // We're inside a method belonging to a class (contextRecord).
    // Is contextRecord a friend of record ? Lets check:

    for (auto fr : record->friends()) {
        TypeSourceInfo *si = fr->getFriendType();
        if (si) {
            const Type *t = si->getType().getTypePtrOrNull();
            CXXRecordDecl *friendClass = t ? t->getAsCXXRecordDecl() : nullptr;
            if (friendClass == contextRecord) {
                return true;
            }
        }
    }

    // There's still hope, lets see if the context is nested inside the class we're trying to access
    // Inner classes can access private members of outter classes.
    DeclContext *it = contextRecord;
    do {
        it = it->getParent();
        if (it == record)
            return true;
    } while (it);

    if (method->getAccess() == clang::AccessSpecifier::AS_private)
        return false;

    if (method->getAccess() != clang::AccessSpecifier::AS_protected) // shouldnt happen, must be protected at this point.
        return false;

    // For protected there's still hope, since record might be a derived or base class
    if (derivesFrom(record, contextRecord))
        return true;

    if (derivesFrom(contextRecord, record)) {
        isSpecialProtectedCase = true;
        return true;
    }

    return false;
}

bool Utils::isConvertibleTo(const Type *source, const Type *target)
{
    if (!source || !target)
        return false;

    if (source->isPointerType() ^ target->isPointerType())
        return false;

    if (source == target)
        return true;

    if (source->getPointeeCXXRecordDecl() && source->getPointeeCXXRecordDecl() == target->getPointeeCXXRecordDecl())
        return true;

    if (source->isIntegerType() && target->isIntegerType())
        return true;

    if (source->isFloatingType() && target->isFloatingType())
        return true;

    return false;
}

CXXRecordDecl* Utils::firstMethodOrClassContext(DeclContext *context)
{
    if (!context)
        return nullptr;

    if (isa<CXXRecordDecl>(context))
        return dyn_cast<CXXRecordDecl>(context);

    return firstMethodOrClassContext(context->getParent());
}

bool Utils::isAscii(StringLiteral *lt)
{
    // 'é' for some reason has isAscii() == true, so also call containsNonAsciiOrNull
    return lt && lt->isAscii() && !lt->containsNonAsciiOrNull();
}

bool Utils::isInDerefExpression(Stmt *s, ParentMap *map)
{
    if (!s)
        return false;

    Stmt *p = s;
    do {
        p = HierarchyUtils::parent(map, p);
        CXXOperatorCallExpr *op = p ? dyn_cast<CXXOperatorCallExpr>(p) : nullptr;
        if (op && op->getDirectCallee() && op->getDirectCallee()->getNameAsString() == "operator*") {
            return op;
        }
    } while (p);

    return false;
}

std::vector<CallExpr *> Utils::callListForChain(CallExpr *lastCallExpr)
{
    if (!lastCallExpr)
        return {};

    const bool isOperator = isa<CXXOperatorCallExpr>(lastCallExpr);

    vector<CallExpr *> callexprs = { lastCallExpr };
    Stmt *s = lastCallExpr;
    do {
        const int childCount = std::distance(s->child_begin(), s->child_end());
        if (isOperator && childCount > 1) {
            // for operator case, the chained call childs are in the second child
            s = *(++s->child_begin());
        } else {
            s = childCount > 0 ? *s->child_begin() : nullptr;
        }

        if (s) {
            CallExpr *callExpr = dyn_cast<CallExpr>(s);
            if (callExpr && callExpr->getCalleeDecl()) {
                callexprs.push_back(callExpr);
            } else if (MemberExpr *memberExpr = dyn_cast<MemberExpr>(s)) {
                if (isa<FieldDecl>(memberExpr->getMemberDecl()))
                    break; // accessing a public member via . or -> breaks the chain
            }
        }
    } while (s);

    return callexprs;
}

CXXRecordDecl *Utils::rootBaseClass(CXXRecordDecl *derived)
{
    if (!derived || derived->getNumBases() == 0)
        return derived;

    CXXBaseSpecifier *base = derived->bases_begin();
    CXXRecordDecl *record = base->getType()->getAsCXXRecordDecl();

    return record ? rootBaseClass(record) : derived;
}

CXXConstructorDecl *Utils::copyCtor(CXXRecordDecl *record)
{
    for (auto ctor : record->ctors()) {
        if (ctor->isCopyConstructor())
            return ctor;
    }

    return nullptr;
}

CXXMethodDecl *Utils::copyAssign(CXXRecordDecl *record)
{
    for (auto copyAssign : record->methods()) {
        if (copyAssign->isCopyAssignmentOperator())
            return copyAssign;
    }

    return nullptr;
}

bool Utils::hasMember(CXXRecordDecl *record, const string &memberTypeName)
{
    if (!record)
        return false;

    for (auto field : record->fields()) {
        field->getParent()->getNameAsString();
        QualType qt = field->getType();
        const Type *t = qt.getTypePtrOrNull();
        if (t && t->getAsCXXRecordDecl()) {
            CXXRecordDecl *rec = t->getAsCXXRecordDecl();
            if (rec->getNameAsString() == memberTypeName)
                return true;
        }
    }

    return false;
}

bool Utils::classifyQualType(const CompilerInstance &ci, const VarDecl *varDecl, QualTypeClassification &classif, clang::Stmt *body)
{
    if (!varDecl)
        return false;

    QualType qualType = unrefQualType(varDecl->getType());
    const Type *paramType = qualType.getTypePtrOrNull();
    if (!paramType || paramType->isIncompleteType())
        return false;

    classif.size_of_T = ci.getASTContext().getTypeSize(qualType) / 8;
    classif.isBig = classif.size_of_T > 16;
    CXXRecordDecl *recordDecl = paramType->getAsCXXRecordDecl();
    classif.isNonTriviallyCopyable = recordDecl && (recordDecl->hasNonTrivialCopyConstructor() || recordDecl->hasNonTrivialDestructor());
    classif.isReference = varDecl->getType()->isLValueReferenceType();
    classif.isConst = qualType.isConstQualified();

    if (varDecl->getType()->isRValueReferenceType()) // && ref, nothing to do here
        return true;

    if (classif.isConst && !classif.isReference) {
        classif.passNonTriviallyCopyableByConstRef = classif.isNonTriviallyCopyable;
        if (classif.isBig) {
            classif.passBigTypeByConstRef = true;
        }
    } else if (classif.isConst && classif.isReference && !classif.isNonTriviallyCopyable && !classif.isBig) {
        classif.passSmallTrivialByValue = true;
    } else if (!classif.isConst && !classif.isReference && (classif.isBig || classif.isNonTriviallyCopyable)) {
        if (body && (Utils::containsNonConstMemberCall(body, varDecl) || Utils::isPassedToFunction(body, varDecl, /*byrefonly=*/ true)))
            return true;
        classif.passNonTriviallyCopyableByConstRef = classif.isNonTriviallyCopyable;
        if (classif.isBig) {
            classif.passBigTypeByConstRef = true;
        }
    }

    return true;
}

QualType Utils::unrefQualType(const QualType &qualType)
{
    const Type *t = qualType.getTypePtrOrNull();
    return (t && t->isReferenceType()) ? t->getPointeeType() : qualType;
}

bool Utils::isSharedPointer(CXXRecordDecl *record)
{
    static const vector<string> names = { "std::shared_ptr", "QSharedPointer", "boost::shared_ptr" };
    return record ? clazy_std::contains(names, record->getQualifiedNameAsString()) : false;
}

bool Utils::isInitializedExternally(clang::VarDecl *varDecl)
{
    if (!varDecl)
        return false;

    DeclContext *context = varDecl->getDeclContext();
    FunctionDecl *fDecl = context ? dyn_cast<FunctionDecl>(context) : nullptr;
    Stmt *body = fDecl ? fDecl->getBody() : nullptr;
    if (!body)
        return false;

    vector<DeclStmt*> declStmts;
    HierarchyUtils::getChilds<DeclStmt>(body, declStmts);
    for (auto it = declStmts.cbegin(), end = declStmts.cend(); it != end; ++it) {
        DeclStmt *declStmt = *it;
        if (declStmt->getSingleDecl() == varDecl) {
            vector<DeclRefExpr*> declRefs;

            HierarchyUtils::getChilds<DeclRefExpr>(declStmt, declRefs);
            if (!declRefs.empty()) {
                return true;
            }

            vector<CallExpr*> callExprs;
            HierarchyUtils::getChilds<CallExpr>(declStmt, callExprs);
            if (!callExprs.empty()) {
                return true;
            }
        }
    }

    return false;
}

bool Utils::functionHasEmptyBody(clang::FunctionDecl *func)
{
    Stmt *body = func ? func->getBody() : nullptr;
    if (!body)
        return false;

    auto it = body->child_begin();
    if (it == body->child_end()) {
        return true;
    }

    if (auto *compound = dyn_cast<CompoundStmt>(*it)) {
        bool isEmpty = compound->child_begin() == compound->child_end();
        if (isEmpty) {
            return true;
        }
    }

    return false;
}
