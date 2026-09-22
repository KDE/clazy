#!/usr/bin/env sh

# SPDX-FileCopyrightText: 2025 Alexander Lohnau <alexander.lohnau@gmx.de>
# SPDX-License-Identifier: LGPL-2.0-or-later

# If user asked for help, show a short wrapper note then delegate to run-clang-tidy help.
help_requested=0
for _arg in "$@"; do
    case "$_arg" in
        -h|--help|help)
            help_requested=1
            break
            ;;
    esac
done

if [ "$help_requested" -eq 1 ]; then
    echo "run-clazy-clang-tidy: wrapper for run-clang-tidy that loads the Clazy plugin"
    echo "Usage: run-clazy-clang-tidy [run-clang-tidy options] [files]"
    echo "This script injects the Clazy plugin and skips mocs_compilation.cpp files."
    echo
    echo "Displaying run-clang-tidy help:"
    run-clang-tidy --help
    exit 0
fi

ClazyClangTidyPluginLib=ClazyClangTidy@CMAKE_SHARED_LIBRARY_SUFFIX@

libdir=$("@READLINK_CMD@" -f "$(dirname ${0})/@BIN_RELATIVE_LIBDIR@")
if ( test -f "$libdir/$ClazyClangTidyPluginLib" )
then
    # find plugin libraries in install dir
    export LD_LIBRARY_PATH=$libdir:$LD_LIBRARY_PATH
    export DYLD_LIBRARY_PATH=$libdir:$DYLD_LIBRARY_PATH
elif ( test -f "$(dirname $0)/lib/$ClazyClangTidyPluginLib" )
then
    # find plugin libraries in build dir
    export LD_LIBRARY_PATH=$(dirname $0)/lib:$LD_LIBRARY_PATH
    export DYLD_LIBRARY_PATH=$(dirname $0)/lib:$DYLD_LIBRARY_PATH
fi

run-clang-tidy -load="$ClazyClangTidyPluginLib" -source-filter='^(?!.*/mocs_compilation\.cpp$)' "$@"
