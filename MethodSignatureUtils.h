#ifndef METHOD_SIGNATURE_UTILS_H
#define METHOD_SIGNATURE_UTILS_H

#include <clang/AST/Decl.h>
#include <string>

inline bool hasCharPtrArgument(clang::FunctionDecl *func, int expected_arguments = -1)
{
    if (expected_arguments != -1 && (int)func->param_size() != expected_arguments)
        return false;

    auto it = func->param_begin();
    auto e = func->param_end();

    for (; it != e; ++it) {
        clang::QualType qt = (*it)->getType();
        const clang::Type *t = qt.getTypePtrOrNull();
        if (t == nullptr)
            continue;

        const clang::Type *realT = t->getPointeeType().getTypePtrOrNull();

        if (realT == nullptr)
            continue;

        if (realT->isCharType())
            return true;
    }

    return false;
}

inline bool hasArgumentOfType(clang::FunctionDecl *func, const std::string &typeName, int expected_arguments = -1)
{
    if (expected_arguments != -1 && (int)func->param_size() != expected_arguments)
        return false;

    auto it = func->param_begin();
    auto e = func->param_end();

    for (; it != e; ++it) {
        clang::QualType qt = (*it)->getType();
        if (qt.getAsString() == typeName.c_str())
            return true;
    }

    return false;
}


#endif
