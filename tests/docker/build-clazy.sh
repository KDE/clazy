# This is the script that runs inside the docker container and builds clazy

BRANCH=$1
J_FLAG=$2
CLAZY_PREFIX=$3

if [ -z "$1" ]
then
    exit 1;
fi

if [ -z "$2" ]
then
    exit 1;
fi

if [ -z "$3" ]
then
    exit 1;
fi

cd /root/clazy && git pull && git checkout $BRANCH && cmake -DCMAKE_INSTALL_PREFIX=$CLAZY_PREFIX -DCMAKE_BUILD_TYPE=RelWithDebInfo . && make $J_FLAG && make install && cd tests && ./run_tests.py
