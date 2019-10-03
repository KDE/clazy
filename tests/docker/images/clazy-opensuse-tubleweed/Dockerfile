# This Dockerfile creates the container for testing on Opensuse Tumbleweed
# You can run, for example: clazy/tests/docker/test_docker.py -b 1.6 , which will run the tests in all containers
# Or explicitly: docker run -i -t iamsergio/clazy-opensuse-tumbleweed sh /root/clazy/tests/docker/build-clazy.sh 1.6 -j12 none /usr

FROM opensuse/tumbleweed
MAINTAINER Sergio Martins (sergio.martins@kdab.com)

RUN zypper update
RUN zypper -n install cmake git-core llvm llvm-devel llvm-clang llvm-clang-devel libqt5-qtbase-devel libqt5-qtdeclarative-devel

WORKDIR /root
RUN git clone https://github.com/KDE/clazy.git
