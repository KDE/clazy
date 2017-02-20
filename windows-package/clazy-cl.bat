@echo off
%~dp0\clang\clang.exe --driver-mode=cl -Qunused-arguments -Xclang -load -Xclang ClangLazy.dll -Xclang -add-plugin -Xclang clang-lazy -Wno-microsoft-enum-value %*
