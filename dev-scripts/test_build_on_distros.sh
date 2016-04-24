BRANCH=$1
J_FLAG=$2
BUILD_SCRIPT=/usr/bin/build-clazy.sh

if [ -z "$1" ]
then
    BRANCH="master"
fi

docker run -i -t iamsergio/clazy-ubuntu-14.04 sh $BUILD_SCRIPT $BRANCH $J_FLAG && \
docker run -i -t iamsergio/clazy-ubuntu-15.10 sh $BUILD_SCRIPT $BRANCH $J_FLAG && \
docker run -i -t iamsergio/clazy-ubuntu-16.04 sh $BUILD_SCRIPT $BRANCH $J_FLAG && \
docker run -i -t iamsergio/clazy-opensuse-tumbleweed sh $BUILD_SCRIPT $BRANCH $J_FLAG && \
docker run -i -t iamsergio/clazy-archlinux sh $BUILD_SCRIPT $BRANCH $J_FLAG
