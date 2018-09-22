# This is the script that runs inside the docker container and builds clazy

BRANCH=$1
J_FLAG=$2
CLAZY_PREFIX=$3
IGNORE_CHECKS=$4
LLVM_ROOT = $5

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

if [ "$IGNORE_CHECKS" = "none" ]
then
    IGNORE_CHECKS = ""
else
    IGNORE_CHECKS = " --exclude ${IGNORE_CHECKS} "
fi

if [ "$5" = "none" ]
then
    unset LLVM_ROOT
fi

export PATH=$CLAZY_PREFIX/bin:$PATH
export LD_LIBRARY_PATH=$CLAZY_PREFIX/lib:$CLAZY_PREFIX/lib64:$LD_LIBRARY_PATH

cd /root/clazy && git fetch && git checkout origin/$BRANCH && cmake -DCMAKE_INSTALL_PREFIX=$CLAZY_PREFIX -DCMAKE_BUILD_TYPE=RelWithDebInfo . && make $J_FLAG && make install && cd tests && ./run_tests.py $IGNORE_CHECKS
