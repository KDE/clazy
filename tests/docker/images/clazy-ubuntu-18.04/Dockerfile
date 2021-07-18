# This Dockerfile creates the container for testing on Ubuntu 18.04


FROM ubuntu:18.04
MAINTAINER Sergio Martins (sergio.martins@kdab.com)

ENV PATH=/Qt/5.15.2/gcc_64/bin/:$PATH
ENV LC_CTYPE=C.UTF-8


RUN apt-get update
RUN apt install -y build-essential g++ clang clang-7 clang-8 clang-tools clang-tools-7 \
clang-tools-8 libclang-dev libclang-7-dev libclang-8-dev git-core python3 \
ninja-build qtbase5-dev qtdeclarative5-dev libssl-dev

# Install a more recent CMake, so we can use presets
WORKDIR /
RUN git clone https://github.com/Kitware/CMake.git
WORKDIR /CMake
RUN git checkout v3.21.0 && ./configure --prefix=/usr/ && make -j10 && make install

RUN groupadd -g 1000 defaultgroup && \
useradd -u 1000 -g defaultgroup user -m

ENV PATH=/usr/lib/llvm-8/bin/:/clazy-src/build-ubuntu-18.04/bin/:$PATH
ENV LD_LIBRARY_PATH=/usr/lib/llvm-8/lib64/:/clazy-src/build-ubuntu-18.04/lib/:$LD_LIBRARY_PATH
ENV CLANG_BUILTIN_INCLUDE_DIR=/usr/lib/llvm-8/lib/clang/8.0.0/include/
