#!/bin/sh
#-------------------------------------------------------------------------------
# Change here, if needed

QT_FLAGS="-I /usr/include/qt/ -fPIC"

#-------------------------------------------------------------------------------

CXX="clang++ -Qunused-arguments -Xclang -load -Xclang ClangMoreWarningsPlugin.so -Xclang -add-plugin -Xclang more-warnings -c ${QT_FLAGS} -Xclang -plugin-arg-more-warnings -Xclang "

for folder in */ ; do
    cd ${folder}
    CURRENT_CHECK_NAME=${PWD##*/}
    echo $CXX $CURRENT_CHECK_NAME main.cpp > compile.output
    $CXX $CURRENT_CHECK_NAME main.cpp -o /tmp/foo.o &>> compile.output

    if [ ! $? ] ; then echo "build error! See ${folder}compile.output" ; exit -1 ; fi

    grep "warning:" compile.output &> test.output

    if ! diff -q test.expected test.output &> /dev/null ; then
        echo "[FAIL] $folder"
        echo
        diff -Naur test.expected test.output
    else
        echo "[OK] $folder"
    fi

    cd ..
done
