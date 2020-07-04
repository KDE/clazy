FROM centos:6.8
MAINTAINER Sergio Martins (sergio.martins@kdab.com)

WORKDIR /
RUN yum update -y
RUN yum install -y git wget zip texinfo bison flex svn boost-regex boost-devel

RUN wget http://people.centos.org/tru/devtools-2/devtools-2.repo -O /etc/yum.repos.d/devtools-2.repo
RUN yum install -y devtoolset-2-gcc devtoolset-2-binutils devtoolset-2-gcc-c++

ENV PATH=/opt/rh/devtoolset-2/root/usr/bin/:$PATH

# Build GCC
WORKDIR /
RUN svn co svn://gcc.gnu.org/svn/gcc/tags/gcc_5_4_0_release/
WORKDIR /gcc_5_4_0_release/
RUN ./contrib/download_prerequisites
RUN mkdir /gcc_5_4_0_release_build/
WORKDIR /gcc_5_4_0_release_build/
RUN ../gcc_5_4_0_release/configure --disable-multilib
RUN make -j12
RUN make install
RUN echo "/usr/local/lib64" > /etc/ld.so.conf.d/newStdLib.conf
RUN ldconfig

ENV CXX=/usr/local/bin/g++
ENV CC=/usr/local/bin/gcc

# Copy appdir template
ADD clazy.AppDir /clazy.AppDir/

# Build Python
WORKDIR /
RUN wget https://www.python.org/ftp/python/2.7.16/Python-2.7.16.tgz
RUN tar xzf Python-2.7.16.tgz
WORKDIR /Python-2.7.16
RUN ./configure --enable-optimizations
RUN make -j8 altinstall
RUN ln -sf /usr/local/bin/python2.7 /usr/local/bin/python

#Build CMake
WORKDIR /
RUN git clone https://github.com/Kitware/CMake.git
WORKDIR /CMake
RUN git checkout v3.15.4
RUN ./configure --prefix=/usr/
RUN make -j8
RUN make install

#Build LLVM
WORKDIR /
RUN wget -O - https://github.com/llvm/llvm-project/tarball/llvmorg-10.0.0 | tar xz
RUN mv llvm-llvm-project-* llvm-project
WORKDIR /llvm-project/
RUN echo "LLVM sha1: `git rev-parse HEAD`" > /clazy.AppDir/sha1

RUN mkdir /root/build_llvm/
WORKDIR /root/build_llvm/
RUN cmake -DCMAKE_INSTALL_PREFIX=/usr/ -DLLVM_INCLUDE_EXAMPLES=OFF -DLLVM_TARGETS_TO_BUILD="X86" -DLLVM_ENABLE_PROJECTS="clang;clang-tools-extra" -DBUILD_SHARED_LIBS=OFF -DCMAKE_BUILD_TYPE=Release -DLLVM_TEMPORARILY_ALLOW_OLD_TOOLCHAIN=ON -DCLANG_ENABLE_CLANGD=OFF /llvm-project/llvm/
RUN make -j8
RUN make install
RUN make DESTDIR=/clazy.AppDir install
RUN rm -rf /clazy.AppDir/usr/include/
RUN rm -rf /clazy.AppDir/usr/share/
RUN rm -rf /clazy.AppDir/usr/lib/*.a
RUN rm -rf /clazy.AppDir/usr/lib/*.so*
RUN rm -rf /clazy.AppDir/usr/lib/cmake
RUN rm -rf /clazy.AppDir/usr/bin/ll*
WORKDIR /clazy.AppDir/usr/bin/
RUN rm -rf bugpoint clang-change-namespace clang-doc clang-format verify-uselistorder c-index-test clang-include-fixer clang-query clang-rename clang-scan-deps diagtool find-all-symbols hmaptool obj2yaml sancov scan-build verify-uselistorderc-index-test clang-check clang-extdef-mapping clang-import-test clang-offload-bundler clang-refactor clang-reorder-fields clang-tidy dsymutil git-clang-format modularize opt sanstats scan-view yaml2obj
RUN chmod +x *

#Clone clazy
WORKDIR /
RUN git clone https://github.com/KDE/clazy.git
