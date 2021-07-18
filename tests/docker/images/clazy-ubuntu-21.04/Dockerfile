# This Dockerfile creates the container for testing on Ubuntu 21.04

FROM ubuntu:21.04
MAINTAINER Sergio Martins (sergio.martins@kdab.com)


ENV PATH=/Qt/5.15.2/gcc_64/bin/:$PATH
ENV LC_CTYPE=C.UTF-8

ENV TZ=Europe/Lisbon
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

RUN apt-get update
RUN apt install -y build-essential g++ clang-12 clang-tools-12 libclang-12-dev libclang-12-dev \
git-core python3 ninja-build qtbase5-dev qtdeclarative5-dev libssl-dev

# Install a more recent CMake, so we can use presets
WORKDIR /
RUN git clone https://github.com/Kitware/CMake.git
WORKDIR /CMake
RUN git checkout v3.21.0 && ./configure --prefix=/usr/ && make -j10 && make install

RUN groupadd -g 1000 defaultgroup && \
useradd -u 1000 -g defaultgroup user -m

ENV PATH=/usr/lib/llvm-12/bin/:/clazy-src/build-ubuntu-21.04/bin/:$PATH
ENV LD_LIBRARY_PATH=/usr/lib/llvm-12/lib64/:/clazy-src/build-ubuntu-21.04/lib/:$LD_LIBRARY_PATH
ENV CLANG_BUILTIN_INCLUDE_DIR=/usr/lib/llvm-12/lib/clang/12.0.0/include/
