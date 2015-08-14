#ifndef MORE_WARNINGS_STRING_UTILS_H
#define MORE_WARNINGS_STRING_UTILS_H

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

template <typename T>
inline bool isOfClass(T *node, const std::string &className)
{
    return classNameFor<T>(node) == className;
}

#endif

