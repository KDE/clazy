# This Dockerfile creates the container for testing on Fedora 34

FROM fedora:34
MAINTAINER Sergio Martins (sergio.martins@kdab.com)

RUN yum -y update
RUN yum -y install openssl-devel make git ninja-build gcc llvm-devel clang-devel qt5-qtbase-devel qt5-qtdeclarative-devel diffutils

# Install a more recent CMake, so we can use presets
WORKDIR /
RUN git clone https://github.com/Kitware/CMake.git
WORKDIR /CMake
RUN git checkout v3.21.0 && ./configure --prefix=/usr/ && make -j10 && make install
