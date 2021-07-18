# This Dockerfile creates the container for testing on openSUSE Tubleweed

FROM opensuse/tumbleweed
MAINTAINER Sergio Martins (sergio.martins@kdab.com)

RUN zypper -n update
RUN zypper -n install cmake ninja git-core llvm llvm-devel llvm-clang llvm-clang-devel libqt5-qtbase-devel libqt5-qtdeclarative-devel

ENV CLAZY_SRC=/clazy-src
ENV PATH=$CLAZY_SRC/build-opensuse-tumbleweed/bin:$PATH
ENV LD_LIBRARY_PATH=$CLAZY_SRC/build-opensuse-tumbleweed/lib:$LD_LIBRARY_PATH
ENV CLANG_BUILTIN_INCLUDE_DIR=/usr/lib64/clang/12.0.1/include/
