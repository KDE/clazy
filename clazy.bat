@echo off
clang.exe -Qunused-arguments -Xclang -load -Xclang ClazyPlugin.dll -Xclang -add-plugin -Xclang clazy %*
