# This Dockerfile creates the container for testing on Fedora
# You can run, for example: clazy/tests/docker/test_docker.py -b 1.6 , which will run the tests in all containers
# Or explicitly: docker run -i -t iamsergio/clazy-fedora-31 sh /root/clazy/tests/docker/build-clazy.sh 1.6 -j12 none /usr -DLINK_CLAZY_TO_LLVM=OFF

FROM fedora:31
MAINTAINER Sergio Martins (sergio.martins@kdab.com)

RUN yum -y update
RUN yum -y install make cmake git ninja-build gcc llvm-devel clang-devel qt5-qtbase-devel qt5-qtdeclarative-devel diffutils

WORKDIR /root
RUN git clone https://github.com/KDE/clazy.git
