@echo off
%~dp0\clang\clang.exe -Qunused-arguments -Xclang -add-plugin -Xclang clang-lazy %*
