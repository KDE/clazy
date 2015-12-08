@echo off
clang++ -Qunused-arguments -Xclang -load -Xclang ClangLazy.dll -Xclang -add-plugin -Xclang clang-lazy %*
