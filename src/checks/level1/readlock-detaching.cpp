/*
    Copyright (C) 2025 Author <your@email>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "readlock-detaching.h"
#include "QtUtils.h"
#include "TypeUtils.h"
#include "Utils.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/ExprCXX.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/OperatorKinds.h"

using namespace clang;
using namespace clang::ast_matchers;

template<typename T>
const T *getParentOfTypeRecursive(const DynTypedNode &node, ASTContext &context, int depth = 0)
{
    if (depth > 20) // avoid infinite recursion
        return nullptr;

    if (const T *result = node.get<T>()) {
        return result;
    }
    auto parents = context.getParents(node);
    for (const auto &parent : parents) {
        if (const T *result = parent.get<T>())
            return result;
        if (const T *recursive = getParentOfTypeRecursive<T>(parent, context, depth + 1))
            return recursive;
    }

    return nullptr;
}

bool isWithinRange(const SourceRange callLocRange, SourceRange range, const SourceManager &SM)
{
    return !SM.isBeforeInTranslationUnit(callLocRange.getBegin(), range.getBegin()) && !SM.isBeforeInTranslationUnit(range.getEnd(), callLocRange.getEnd());
}

using namespace clang;

class MemberCallVisitor : public RecursiveASTVisitor<MemberCallVisitor>
{
public:
    explicit MemberCallVisitor(ASTContext *Context, CheckBase *check, SourceRange lockRange)
        : Context(Context)
        , check(check)
        , lockRange(lockRange)
    {
    }

    bool VisitCXXOperatorCallExpr(CXXOperatorCallExpr const *call)
    {
        // We only care about members for now
        if (!llvm::isa<MemberExpr>(call->getArg(0))) {
            return true;
        }

        std::string className = "";
        if (const auto *memberExpr = dyn_cast<MemberExpr>(call->getArg(0))) {
            QualType qt = memberExpr->getType(); // 'QMap<QString, QString>'
            if (qt->isRecordType()) {
                className = qt->getAs<RecordType>()->getDecl()->getNameAsString();
            }
        }
        if (className.empty()) {
            return true;
        }

        const auto methods = clazy::detachingMethodsWithConstCounterParts();
        const auto method = methods.find(className);
        if (method == methods.end()) {
            return true;
        }
        const auto methodName = std::string("operator") + getOperatorSpelling(call->getOperator());
        if (std::find(method->second.begin(), method->second.end(), methodName) == method->second.end()) {
            return true;
        }

        if (!isWithinRange(call->getSourceRange(), lockRange, Context->getSourceManager())) {
            return true;
        }

        check->emitWarning(call, "Possibly detaching a member while inside of a read-only mutex scope", true);

        return true;
    }
    bool VisitCXXMemberCallExpr(CXXMemberCallExpr const *call)
    {
        const auto recordName = call->getRecordDecl()->getNameAsString();
        // We only care about members for now
        if (!llvm::isa<MemberExpr>(call->getImplicitObjectArgument()) && recordName != "QReadLocker" && recordName != "QReadWriteLock") {
            return true;
        }

        const auto methods = clazy::detachingMethodsWithConstCounterParts();
        const auto methodName = clazy::name(call->getMethodDecl());
        if ((recordName == "QReadLocker" || recordName == "QReadWriteLock") && methodName == "unlock") {
            lockRange.setEnd(call->getEndLoc());
            return true;
        }

        const auto method = methods.find(recordName);

        if (method == methods.end()) {
            return true;
        }
        if (std::find(method->second.begin(), method->second.end(), methodName) == method->second.end()) {
            return true;
        }

        if (!isWithinRange(call->getSourceRange(), lockRange, Context->getSourceManager())) {
            return true;
        }

        check->emitWarning(call, "Possibly detaching a member while inside of a read-only mutex scope", true);

        return true;
    }

private:
    ASTContext *const Context;
    CheckBase *const check;
    SourceRange lockRange;
};

class ReadlockDetaching_Callback : public ClazyAstMatcherCallback
{
public:
    using ClazyAstMatcherCallback::ClazyAstMatcherCallback;

    void run(const MatchFinder::MatchResult &result) override
    {
        const auto getSorroundingCompondStmt = [&result](auto &expr) -> const CompoundStmt * {
            const auto parents = result.Context->getParents(expr);
            for (auto parent : parents) {
                if (auto surroundingFnc = getParentOfTypeRecursive<CompoundStmt>(parent, *result.Context)) {
                    return surroundingFnc;
                }
            }
            return nullptr;
        };

        if (const auto *constructExpr = result.Nodes.getNodeAs<CXXConstructExpr>("qreadlockerCtor")) {
            if (const auto surroundingStmt = getSorroundingCompondStmt(*constructExpr)) {
                MemberCallVisitor visitor(result.Context, m_check, SourceRange(constructExpr->getBeginLoc(), surroundingStmt->getEndLoc()));
                visitor.TraverseStmt(const_cast<CompoundStmt *>(surroundingStmt));
            }
        }

        if (const auto lockCall = result.Nodes.getNodeAs<CXXMemberCallExpr>("qreadwritelockCall")) {
            const auto surroundingStmt = getSorroundingCompondStmt(*lockCall);
            // The end location for sure needs to be adjusted within the visitor. We just assume it is going to be in the same block the lock was locked
            MemberCallVisitor visitor(result.Context, m_check, SourceRange(lockCall->getBeginLoc(), surroundingStmt->getEndLoc()));
            visitor.TraverseStmt(const_cast<CompoundStmt *>(surroundingStmt));
        }
    }
};
ReadlockDetaching::ReadlockDetaching(const std::string &name, ClazyContext *context)
    : CheckBase(name, context)
    , m_astMatcherCallBack(new ReadlockDetaching_Callback(this))
{
}

void ReadlockDetaching::registerASTMatchers(MatchFinder &finder)
{
    finder.addMatcher(cxxConstructExpr(hasType(cxxRecordDecl(hasName("QReadLocker")))).bind("qreadlockerCtor"), m_astMatcherCallBack);
    finder.addMatcher(
        cxxMemberCallExpr(on(hasType(cxxRecordDecl(hasName("QReadWriteLock")))), callee(cxxMethodDecl(hasName("lockForRead")))).bind("qreadwritelockCall"),
        m_astMatcherCallBack);
}
