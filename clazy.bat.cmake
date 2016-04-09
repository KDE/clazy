@echo off
${CLANG_COMPILER} -Qunused-arguments -Xclang -load -Xclang ClangLazy.dll -Xclang -add-plugin -Xclang clang-lazy %*
