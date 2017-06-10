BRANCH=$1
J_FLAG=$2
BUILD_SCRIPT=/usr/bin/build-clazy.sh

if [ -z "$1" ]
then
    BRANCH="master"
fi

function run_test
{
    echo "Testing $1..."
    docker run -i -t iamsergio/clazy-$1 sh $BUILD_SCRIPT $BRANCH $J_FLAG &> clazy-$1.log
    echo $?
}

run_test ubuntu-17.04
run_test ubuntu-16.04
run_test opensuse-tumbleweed
run_test archlinux
