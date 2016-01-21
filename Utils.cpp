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

    for (auto it = childDecl->bases_begin(), e = childDecl->bases_end(); it != e; ++it) {
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
    for (auto it = decl->ctor_begin(), e = decl->ctor_end(); it != e; ++it) {
        CXXConstructorDecl *ctor = *it;
        if (ctor->isConstexpr())
            return true;
    }
    return false;
}

ClassTemplateSpecializationDecl *Utils::templateDecl(Decl *decl)
{
    VarDecl *varDecl = dyn_cast<VarDecl>(decl);
    if (!varDecl) return nullptr;
    QualType qt = varDecl->getType();
    const Type *t = qt.getTypePtrOrNull();
    if (!t) return nullptr;
    CXXRecordDecl *classDecl = t->getAsCXXRecordDecl();
    if (!classDecl) return nullptr;
    return dyn_cast<ClassTemplateSpecializationDecl>(classDecl);
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
    HierarchyUtils::getChilds<MemberExpr>(implicitObject, memberExprs);
    HierarchyUtils::getChilds<DeclRefExpr>(implicitObject, declRefs);

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
    return context != nullptr && isa<FunctionDecl>(context);
}

bool Utils::loopCanBeInterrupted(clang::Stmt *stmt, const clang::CompilerInstance &ci, const clang::SourceLocation &onlyBeforeThisLoc)
{
    if (stmt == nullptr)
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


bool Utils::containsNonConstMemberCall(Stmt *body, const VarDecl *varDecl)
{
    std::vector<CXXMemberCallExpr*> memberCalls;
    HierarchyUtils::getChilds2<CXXMemberCallExpr>(body, memberCalls);

    for (auto it = memberCalls.cbegin(), end = memberCalls.cend(); it != end; ++it) {
        CXXMemberCallExpr *memberCall = *it;
        CXXMethodDecl *methodDecl = memberCall->getMethodDecl();
        if (methodDecl == nullptr || methodDecl->isConst())
            continue;

        ValueDecl *valueDecl = Utils::valueDeclForMemberCall(*it);
        if (valueDecl == nullptr)
            continue;

        if (valueDecl == varDecl)
            return true;
    }

    // Check for operator calls:
    std::vector<CXXOperatorCallExpr*> operatorCalls;
    HierarchyUtils::getChilds2<CXXOperatorCallExpr>(body, operatorCalls);
    for (auto it = operatorCalls.cbegin(), end = operatorCalls.cend(); it != end; ++it) {
        CXXOperatorCallExpr *operatorExpr = *it;
        FunctionDecl *fDecl = operatorExpr->getDirectCallee();
        if (fDecl == nullptr)
            continue;
        CXXMethodDecl *methodDecl = dyn_cast<CXXMethodDecl>(fDecl);
        if (methodDecl == nullptr || methodDecl->isConst())
            continue;

        ValueDecl *valueDecl = Utils::valueDeclForOperatorCall(*it);
        if (valueDecl == nullptr)
            continue;

        if (valueDecl == varDecl)
            return true;
    }

    return false;
}

template<class T>
static bool argByRef(T expr, FunctionDecl *fDecl, const VarDecl *varDecl)
{
    unsigned int param = 0;
    for (auto arg = expr->arg_begin(), arg_end = expr->arg_end(); arg != arg_end; ++arg) {
        DeclRefExpr *refExpr = dyn_cast<DeclRefExpr>(*arg);
        if (refExpr == nullptr)  {

            if ((*arg)->child_begin() != (*arg)->child_end()) {
                refExpr = dyn_cast<DeclRefExpr>(*((*arg)->child_begin()));
                if (refExpr == nullptr)
                    continue;
            } else {
                continue;
            }
        }

        if (refExpr->getDecl() != varDecl) // It's our variable ?
            continue;

        // It is, lets see if the callee takes our variable by const-ref
        if (param >= fDecl->param_size())
            continue;

        ParmVarDecl *paramDecl = fDecl->getParamDecl(param);
        if (paramDecl == nullptr)
            continue;

        QualType qt = paramDecl->getType();
        const Type *t = qt.getTypePtrOrNull();
        if (t == nullptr)
            continue;

        if ((t->isReferenceType() || t->isPointerType()) && !t->getPointeeType().isConstQualified())
            return true; // function receives non-const ref, so our foreach variable cant be const-ref

        ++param;
    }

    return false;
}

bool Utils::containsCallByRef(Stmt *body, const VarDecl *varDecl)
{
    std::vector<CallExpr*> callExprs;
    HierarchyUtils::getChilds2<CallExpr>(body, callExprs);
    for (auto it = callExprs.cbegin(), end = callExprs.cend(); it != end; ++it) {
        CallExpr *callexpr = *it;
        FunctionDecl *fDecl = callexpr->getDirectCallee();
        if (fDecl == nullptr)
            continue;

        if (argByRef(callexpr, fDecl, varDecl))
            return true;
    }

    std::vector<CXXConstructExpr*> constructExprs;
    HierarchyUtils::getChilds2<CXXConstructExpr>(body, constructExprs);
    for (auto it = constructExprs.cbegin(), end = constructExprs.cend(); it != end; ++it) {
        CXXConstructExpr *constructExpr = *it;
        FunctionDecl *fDecl = constructExpr->getConstructor();

        if (argByRef(constructExpr, fDecl, varDecl))
            return true;
    }

    return false;
}


bool Utils::containsAssignment(Stmt *body, const VarDecl *varDecl)
{
    // Check for operator calls:
    std::vector<CXXOperatorCallExpr*> operatorCalls;
    HierarchyUtils::getChilds2<CXXOperatorCallExpr>(body, operatorCalls);
    for (auto it = operatorCalls.cbegin(), end = operatorCalls.cend(); it != end; ++it) {
        CXXOperatorCallExpr *operatorExpr = *it;
        FunctionDecl *fDecl = operatorExpr->getDirectCallee();
        if (fDecl != nullptr) {
            CXXMethodDecl *methodDecl = dyn_cast<CXXMethodDecl>(fDecl);
            if (methodDecl != nullptr && methodDecl->isCopyAssignmentOperator()) {
                ValueDecl *valueDecl = Utils::valueDeclForOperatorCall(operatorExpr);
                if (valueDecl == nullptr)
                    continue;

                if (valueDecl == varDecl)
                    return true;
            }
        }
    }

    return false;
}

std::vector<std::string> Utils::splitString(const char *str, char separator)
{
    if (!str)
        return {};

    return splitString(string(str), separator);
}

std::vector<std::string> Utils::splitString(const string &str, char separator)
{
    std::string token;
    std::vector<std::string> result;
    std::istringstream istream(str);
    while (std::getline(istream, token, separator)) {
        result.push_back(token);
    }

    return result;
}

bool Utils::callHasDefaultArguments(clang::CallExpr *expr)
{
    std::vector<clang::CXXDefaultArgExpr*> exprs;
    HierarchyUtils::getChilds2<clang::CXXDefaultArgExpr>(expr, exprs, 1);
    return !exprs.empty();
}


bool Utils::containsStringLiteral(Stmt *stm, bool allowEmpty, int depth)
{
    if (stm == nullptr)
        return false;

    std::vector<StringLiteral*> stringLiterals;
    HierarchyUtils::getChilds2<StringLiteral>(stm, stringLiterals, depth);

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
    for (auto it = ternary->child_begin(), e = ternary->child_end(); it != e; ++it) {
        if (skipFirst) {
            skipFirst = false;
            continue;
        }

        if (isa<StringLiteral>(*it))
            continue;

        auto arrayToPointerDecay = dyn_cast<ImplicitCastExpr>(*it);
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
                if (record && find(anyOf.cbegin(), anyOf.cend(), record->getNameAsString()) != anyOf.cend())
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
    if (expr && expr->getConstructor()) {
        if (find(anyOf.cbegin(), anyOf.cend(), expr->getConstructor()->getNameAsString()) != anyOf.cend())
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

    for (auto it = record->method_begin(), e = record->method_end(); it != e; ++it) {
        CXXMethodDecl *method = *it;
        if (method->getNameAsString() == methodName)
            methods.push_back(method);
    }

    // Also include the base classes
    for (auto it = record->bases_begin(), e = record->bases_end(); it != e; ++it) {

        const Type *t = (*it).getType().getTypePtrOrNull();
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
            auto it = find_if(methodContexts.begin(), methodContexts.end(), [context](DeclContext *c) {
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

    for (auto it = record->friend_begin(), e = record->friend_end(); it != e; ++it) {
        FriendDecl *fr = *it;
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
    if (isChildOf(record, contextRecord))
        return true;

    if (isChildOf(contextRecord, record)) {
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

bool Utils::isChildOf(Stmt *child, Stmt *parent)
{
    if (!child || !parent)
        return false;


    for (auto c = parent->child_begin(), end = parent->child_end(); c != end; ++c) {
        if (*c == child)
            return true;
        if (isChildOf(child, *c))
            return true;
    }

    return false;
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

    return record == nullptr ? derived : rootBaseClass(record);
}

CXXConstructorDecl *Utils::copyCtor(CXXRecordDecl *record)
{
    for (auto it = record->ctor_begin(), end = record->ctor_end(); it != end; ++it) {
        CXXConstructorDecl *ctor = *it;
        if (ctor->isCopyConstructor())
            return ctor;
    }

    return nullptr;
}

CXXMethodDecl *Utils::copyAssign(CXXRecordDecl *record)
{
    for (auto it = record->method_begin(), end = record->method_end(); it != end; ++it) {
        CXXMethodDecl *copyAssign = *it;
        if (copyAssign->isCopyAssignmentOperator())
            return copyAssign;
    }

    return nullptr;
}

bool Utils::hasMember(CXXRecordDecl *record, const string &memberTypeName)
{
    if (!record)
        return false;

    for (auto it = record->field_begin(), end = record->field_end(); it != end; ++it) {
        FieldDecl *field = *it;
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
        if (body && (Utils::containsNonConstMemberCall(body, varDecl) || Utils::containsCallByRef(body, varDecl)))
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
    return record ? find(names.cbegin(), names.cend(), record->getQualifiedNameAsString()) != names.cend()
                  : false;
}

bool Utils::isInMacro(const CompilerInstance &ci, SourceLocation loc, const string &macroName)
{
    if (loc.isValid() && loc.isMacroID()) {
        auto macro = Lexer::getImmediateMacroName(loc, ci.getSourceManager(), ci.getLangOpts());
        return macro == macroName;
    }

    return false;
}
