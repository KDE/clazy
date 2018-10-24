@echo off
clang.exe --driver-mode=cl -Qunused-arguments -Xclang -load -Xclang ClazyPlugin.dll -Xclang -add-plugin -Xclang clazy -Wno-microsoft-enum-value %*
