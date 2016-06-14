#!/usr/bin/env sh

libdir=@CMAKE_INSTALL_PREFIX@/@CMAKE_INSTALL_LIBDIR@
sharedir=@CMAKE_INSTALL_PREFIX@/@SHARE_INSTALL_DIR@/clazy

HELP() {
  echo "Usage: `basename $0` [options] [clang++-options]"
  echo
  echo "Static analyzer for C++/Qt code"
  echo
  echo "Options:"
  echo "  --help             print program help"
  echo "  --list             print a list of all available checkers, arranged by level"
  echo "  --explain [regexp] print explanations for the checker matching a regexp"
  echo "or"
  echo "  --explain          print explanations for all checkers"
  echo
  echo "Any of the options above will print the requested information and then exit."
  echo "Otherwise, options are passed directly to clang++ and handled from there."
  echo
  echo "See the clang++ manual for a list of the very large set of options available"
  echo
}

PRLIST() {
  echo ""
  echo "Checks from level$1. $2:"
  ls -1 $sharedir/doc/level$1/README* | awk -F/ '{printf("    %s\n", $NF)}' | sed s/README-// | sed s/\.md$// | sort
}

PRINFO() {
  lst=`ls -1 $sharedir/doc/level*/README*$1*`
  for f in $lst
  do
    l=`echo $f | awk -F/ '{foo=NF-1; printf("    %s:%s\n", $foo,$NF)}'`
    level=`echo $l | cut -d: -f1`
    checker=`echo $l | cut -d: -f2 | sed s/README-// | sed s/\.md$//`
    echo "== Explanation for checker $checker ($level) =="
    cat $f
    echo
  done
}

if ( test $# -gt 0 -a "$1" = "--help" )
then
  HELP
  exit
fi

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
