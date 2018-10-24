@echo off
"%~dp0\clang\clang.exe" -Qunused-arguments -Xclang -load -Xclang ClazyPlugin.dll -Xclang -add-plugin -Xclang clazy %*
