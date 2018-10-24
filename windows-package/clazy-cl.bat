@echo off
"%~dp0\clang\clang.exe" --driver-mode=cl -Qunused-arguments -Xclang -load -Xclang ClazyPlugin.dll -Xclang -add-plugin -Xclang clazy -Wno-microsoft-enum-value %*
