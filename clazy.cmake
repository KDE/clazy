#!/usr/bin/env sh

#TODO help command line option
#TODO version command line option

libdir=@CMAKE_INSTALL_PREFIX@/@CMAKE_INSTALL_LIBDIR@
sharedir=@CMAKE_INSTALL_PREFIX@/@SHARE_INSTALL_DIR@/clazy

PRLIST() {
  echo ""
  echo "Checks from level$1. $2:"
  ls -1 $sharedir/doc/level$1/README* | awk -F/ '{printf("    %s\n", $NF)}' | sed s/README-// | sort
}

PRINFO() {
    lst=`ls -1 $sharedir/doc/level*/README*$1*`
    for f in $lst
    do
        l=`echo $f | awk -F/ '{foo=NF-1; printf("    %s:%s\n", $foo,$NF)}'`
        level=`echo $l | cut -d: -f1`
        checker=`echo $l | cut -d: -f2 | sed s/README-//`
        echo "== Explanation for checker $checker ($level) =="
        cat $f
        echo
    done
}

if ( test $# -gt 0 -a "$1" = "--list" )
then
  echo "List of available clazy checkers:"
  PRLIST 0 "Very stable checks, 100% safe, no false-positives"
  PRLIST 1 "Mostly stable and safe, rare false-positives"
  PRLIST 2 "Sometimes has false-positives (20-30%)"
  PRLIST 3 "Not always correct, high rate of false-positives"
  exit
fi

if ( test $# -gt 0 -a "$1" = "--explain" )
then
  shift
  PRINFO $@
  exit
fi

${CLANGXX:-clang++} -Qunused-arguments -Xclang -load -Xclang ClangLazy.so -Xclang -add-plugin -Xclang clang-lazy $@
