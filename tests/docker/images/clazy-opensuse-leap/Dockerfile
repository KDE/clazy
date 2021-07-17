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
