# This Dockerfile creates the container for testing on openSUSE Leap
FROM opensuse/leap
MAINTAINER Sergio Martins (sergio.martins@kdab.com)

RUN zypper -n update
RUN zypper -n install -y openssl-devel git-core llvm llvm-devel llvm-clang llvm-clang-devel libqt5-qtbase-devel libqt5-qtdeclarative-devel ninja

# Install a more recent CMake, so we can use presets
WORKDIR /
RUN git clone https://github.com/Kitware/CMake.git
WORKDIR /CMake
RUN git checkout v3.21.0 && ./configure --prefix=/usr/ && make -j5 && make install

ENV CLAZY_SRC=/clazy-src
ENV PATH=$CLAZY_SRC/build-opensuse-leap/bin:$PATH
ENV LD_LIBRARY_PATH=$CLAZY_SRC/build-opensuse-leap/lib:$LD_LIBRARY_PATH
ENV CLANG_BUILTIN_INCLUDE_DIR=/usr/lib64/clang/11.0.1/include/
