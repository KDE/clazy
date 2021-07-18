# This Dockerfile creates the container for testing on Fedora 34

FROM fedora:34
MAINTAINER Sergio Martins (sergio.martins@kdab.com)

RUN yum -y update
RUN yum -y install openssl-devel make git ninja-build gcc llvm-devel clang-devel qt5-qtbase-devel qt5-qtdeclarative-devel diffutils which

# Install a more recent CMake, so we can use presets
WORKDIR /
RUN git clone https://github.com/Kitware/CMake.git
WORKDIR /CMake
RUN git checkout v3.21.0 && ./configure --prefix=/usr/ && make -j10 && make install

RUN groupadd -g 1000 defaultgroup && \
useradd -u 1000 -g defaultgroup user -m

ENV PATH=/usr/lib/llvm-12/bin/:/clazy-src/build-fedora-34/bin/:$PATH
ENV LD_LIBRARY_PATH=/usr/lib/llvm-12/lib64/:/clazy-src/build-fedora-34/lib/:$LD_LIBRARY_PATH
ENV CLANG_BUILTIN_INCLUDE_DIR=/usr/lib64/clang/12.0.0/include/
