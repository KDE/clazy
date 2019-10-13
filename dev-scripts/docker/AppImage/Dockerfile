FROM centos:6.8
MAINTAINER Sergio Martins (sergio.martins@kdab.com)

WORKDIR /
RUN yum update -y
RUN yum install -y git wget

RUN wget http://people.centos.org/tru/devtools-2/devtools-2.repo -O /etc/yum.repos.d/devtools-2.repo
RUN yum install -y devtoolset-2-gcc devtoolset-2-binutils devtoolset-2-gcc-c++

ENV PATH=/opt/rh/devtoolset-2/root/usr/bin/:$PATH


# Build Python
RUN wget https://www.python.org/ftp/python/2.7.16/Python-2.7.16.tgz
RUN tar xzf Python-2.7.16.tgz
WORKDIR /Python-2.7.16
RUN ./configure --enable-optimizations
RUN make  altinstall
RUN ln -sf /usr/local/bin/python2.7 /usr/local/bin/python

#Build CMake
WORKDIR /
RUN git clone https://github.com/Kitware/CMake.git
WORKDIR /CMake
RUN git checkout v3.15.4
RUN ./configure --prefix=/usr/
RUN make -j6
RUN make install

#Build LLVM
WORKDIR /
RUN git clone https://github.com/llvm/llvm-project.git
WORKDIR /llvm-project/
RUN git checkout llvmorg-9.0.0
RUN mkdir /root/build_llvm/
WORKDIR /root/build_llvm/
RUN  cmake -DCMAKE_INSTALL_PREFIX=/usr/  -DLLVM_INCLUDE_EXAMPLES=OFF -DLLVM_TARGETS_TO_BUILD="X86" -DLLVM_ENABLE_PROJECTS="clang;clang-tools-extra" -DBUILD_SHARED_LIBS=OFF -DCMAKE_BUILD_TYPE=Release -DLLVM_TEMPORARILY_ALLOW_OLD_TOOLCHAIN=ON -DCLANG_ENABLE_CLANGD=OFF   /llvm-project/llvm/
RUN make -j6
RUN make install

#Build clazy
WORKDIR /
RUN git clone https://github.com/KDE/clazy.git
