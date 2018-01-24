@echo off
"%~dp0\clang\clang.exe" -Qunused-arguments -Xclang -load -Xclang ClangLazy.dll -Xclang -add-plugin -Xclang clang-lazy %*
