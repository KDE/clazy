#ifndef MORE_WARNINGS_STRING_UTILS_H
#define MORE_WARNINGS_STRING_UTILS_H

#include "checkmanager.h"

#include <clang/AST/ExprCXX.h>
#include <clang/AST/DeclCXX.h>
#include <string>

template <typename T>
inline std::string classNameFor(T *ctorDecl)
{
    return "";
}

template <>
inline std::string classNameFor(clang::CXXConstructorDecl *ctorDecl)
{
    return ctorDecl->getParent()->getNameAsString();
}

template <>
inline std::string classNameFor(clang::CXXMethodDecl *method)
{
    return method->getParent()->getNameAsString();
}

template <>
inline std::string classNameFor(clang::CXXConstructExpr *expr)
{
    return classNameFor(expr->getConstructor());
}

template <typename T>
inline bool isOfClass(T *node, const std::string &className)
{
    return node && classNameFor<T>(node) == className;
}

namespace StringUtils {

inline bool functionIsOneOf(clang::FunctionDecl *func, const std::vector<std::string> &functionNames)
{
    return func && std::find(functionNames.cbegin(), functionNames.cend(), func->getNameAsString()) != functionNames.cend();
}

inline void printLocation(const clang::SourceLocation &loc, bool newLine = true)
{
    llvm::errs() << loc.printToString(*(CheckManager::instance()->m_sm));
    if (newLine)
        llvm::errs() << "\n";
}

inline void printRange(const clang::SourceRange &range, bool newLine = true)
{
    printLocation(range.getBegin(), false);
    llvm::errs() << "-";
    printLocation(range.getEnd(), newLine);
}

inline void printLocation(const clang::Stmt *s, bool newLine = true)
{
    if (s)
        printLocation(s->getLocStart(), newLine);
}

}

#endif

