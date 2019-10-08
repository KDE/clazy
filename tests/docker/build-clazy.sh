# This is the script that runs inside the docker container and builds clazy

BRANCH=$1
J_FLAG=$2
IGNORE_CHECKS=$3
LLVM_ROOT=$4
EXTRA_CMAKE_ARGS=$5

if [ "$IGNORE_CHECKS" = "none" ]
then
    IGNORE_CHECKS=""
else
    IGNORE_CHECKS=" --exclude ${IGNORE_CHECKS} "
fi

export PATH=$LLVM_ROOT/bin:$PATH
export LD_LIBRARY_PATH=$LLVM_ROOT/lib:$LLVM_ROOT/lib64:$LD_LIBRARY_PATH

cd /root/clazy && git fetch && git checkout origin/$BRANCH && cmake -DCMAKE_INSTALL_PREFIX=$LLVM_ROOT -DCMAKE_BUILD_TYPE=RelWithDebInfo $EXTRA_CMAKE_ARGS . && make $J_FLAG && make install && cd tests && ./run_tests.py $IGNORE_CHECKS
