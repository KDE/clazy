#!/usr/bin/env sh

libdir=$("@READLINK_CMD@" -f "$(dirname ${0})/@BIN_RELATIVE_LIBDIR@")
sharedir=$("@READLINK_CMD@" -f "$(dirname ${0})/@BIN_RELATIVE_SHAREDIR@")

HELP() {
  echo "Usage: `basename $0` [options] [clang++-options]"
  echo
  echo "Static analyzer for C++/Qt code (https://phabricator.kde.org/source/clazy)"
  echo
  echo "Options:"
  echo "  --help             print program help"
  echo "  --version          print the program version"
  echo "  --standalone       run clazy-standalone instead of clang"
  echo "  --list             print a list of all available checkers, arranged by level"
  echo "  --explain [regexp] print explanations for the checker matching a regexp"
  echo "or"
  echo "  --explain          print explanations for all checkers"
  echo
  echo "Any of the options above will print the requested information and then exit."
  echo
  echo "Convenience Options:"
  echo "  --qt4compat        Qt4 compatibility mode. useful for source code that can build with Qt4"
  echo "  (this is the same as passing \"-Xclang -plugin-arg-clazy -Xclang qt4-compat\")"
  echo "  --qtdeveloper      Special option for building Qt5 itself resulting in fewer false positives"
  echo "  (this is the same as passing \"-Xclang -plugin-arg-clazy -Xclang qt-developer\")"
  echo
  echo "All other options are passed directly to clang++ and handled from there."
  echo
  echo "See the clang++ manual for a list of the very large set of options available"
  echo
}

VERSION() {
    echo "clazy version: @CLAZY_PRINT_VERSION@"
    ${CLANGXX:-clang++} --version | head -1 | awk '{printf("clang version: %s\n",$3)}'
}

PRLIST() {
  echo ""
  echo "Checks from level$1. $2:"
  ls -1 $sharedir/doc/clazy/level$1/README* | awk -F/ '{printf("    %s\n", $NF)}' | sed s/README-// | sed s/\.md$// | sort
}

PRINFO() {
  lst=`ls -1 $sharedir/doc/clazy/level*/README*$1* $sharedir/doc/clazy/manuallevel/README*$1* 2>/dev/null`
  if ( test ! -z "$lst" )
  then
    for f in $lst
    do
      l=`echo $f | awk -F/ '{foo=NF-1; printf("    %s:%s\n", $foo,$NF)}'`
      level=`echo $l | cut -d: -f1`
      checker=`echo $l | cut -d: -f2 | sed s/README-// | sed s/\.md$//`
      echo "== Explanation for checker $checker ($level) =="
      cat $f
      echo
    done
  else
    echo "There is no explanation available for checker \"$1\""
    echo "Run 'clazy --explain' to see the list of all available checkers."
  fi
}

if ( test $# -gt 0 -a "$1" = "--help" )
then
  HELP
  exit
fi

if ( test $# -gt 0 -a "$1" = "--version" )
then
  VERSION
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

ExtraClangOptions=""
if ( test $# -gt 0 -a "$1" = "--qt4compat" )
then
  shift
  ExtraClangOptions="-Xclang -plugin-arg-clazy -Xclang qt4-compat"
fi
if ( test $# -gt 0 -a "$1" = "--qtdeveloper" )
then
  shift
  ExtraClangOptions="-Xclang -plugin-arg-clazy -Xclang qt-developer"
fi
if ( test $# -gt 0 -a "$1" = "--visit-implicit-code" )
then
  shift
  ExtraClangOptions="-Xclang -plugin-arg-clazy -Xclang visit-implicit-code"
fi

ClazyPluginLib=ClazyPlugin@CMAKE_SHARED_LIBRARY_SUFFIX@

if ( test -f "$libdir/$ClazyPluginLib" )
then
    # find plugin libraries in install dir
    export LD_LIBRARY_PATH=$libdir:$LD_LIBRARY_PATH
    export DYLD_LIBRARY_PATH=$libdir:$DYLD_LIBRARY_PATH
elif ( test -f "$(dirname $0)/lib/$ClazyPluginLib" )
then
    # find plugin libraries in build dir
    export LD_LIBRARY_PATH=$(dirname $0)/lib:$LD_LIBRARY_PATH
    export DYLD_LIBRARY_PATH=$(dirname $0)/lib:$DYLD_LIBRARY_PATH
fi

if ( test $# -gt 0 -a "$1" = "--standalone" )
then
  shift
  if ( test -f "$(dirname $0)/clazy-standalone" )
  then
    # find binary in install dir
    $(dirname $0)/clazy-standalone "$@"
  else
    # hope binary is in the expected build dir
    $(dirname $0)/bin/clazy-standalone "$@"
  fi
else
  ${CLANGXX:-clang++} -Qunused-arguments -Xclang -load -Xclang $ClazyPluginLib -Xclang -add-plugin -Xclang clazy $ExtraClangOptions "$@"
fi
