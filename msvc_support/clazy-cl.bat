@echo off
%~dp0\clang\clang-cl -Qunused-arguments -Xclang -add-plugin -Xclang clang-lazy %*
