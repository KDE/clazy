# This Dockerfile creates the container for testing on Ubuntu
# You can run, for example: clazy/tests/docker/test_docker.py -b 1.6 , which will run the tests in all containers
# Or explicitly: docker run -i -t iamsergio/clazy-ubuntu-18.04 sh /root/clazy/tests/docker/build-clazy.sh 1.6 -j12 none /usr/lib/llvm-6.0/

FROM ubuntu:18.04
MAINTAINER Sergio Martins (sergio.martins@kdab.com)

RUN apt-get update
RUN apt install -y g++ cmake clang clang-7 clang-8 clang-tools clang-tools-7 clang-tools-8 libclang-dev libclang-7-dev libclang-8-dev git-core python3 ninja-build qtbase5-dev qtdeclarative5-dev

WORKDIR /root
RUN git clone https://github.com/KDE/clazy.git
