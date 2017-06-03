*WARNING:* master is the development branch. Please use branch 1.1 for production.

clazy v1.2
===========

clazy is a compiler plugin which allows clang to understand Qt semantics. You get more than 50 Qt related compiler warnings, ranging from unneeded memory allocations to misusage of API, including fix-its for automatic refactoring.

Table of contents
=================

   * [Source Code](#source-code)
   * [Build Instructions](#build-instructions)
      * [Linux](#linux)
         * [Install dependencies](#install-dependencies)
         * [Build and install clang](#build-and-install-clang)
         * [Build clazy](#build-clazy)
      * [Windows](#windows)
         * [Pre-built msvc2015 clang and clazy binaries](#pre-built-msvc2015-clang-and-clazy-binaries)
         * [Build and install clang](#build-and-install-clang-1)
         * [Build clazy](#build-clazy-1)
      * [macOS with MacPorts](#macos-with-macports)
         * [Install clang](#install-clang)
         * [Build clazy](#build-clazy-2)
      * [macOS with Homebrew](#macos-with-homebrew)
         * [Install clang](#install-clang-1)
         * [Build clazy](#build-clazy-3)
   * [Setting up your project to build with clazy](#setting-up-your-project-to-build-with-clazy)
   * [List of checks](#list-of-checks)
   * [Selecting which checks to enable](#selecting-which-checks-to-enable)
      * [Example via env variable](#example-via-env-variable)
      * [Example via compiler argument](#example-via-compiler-argument)
   * [clang-standalone and JSON database support](#clang-standalone-and-json-database-support)
   * [Enabling Fixits](#enabling-fixits)
   * [Troubleshooting](#troubleshooting)
   * [Qt4 compatibility mode](#qt4-compatibility-mode)
   * [Reducing warning noise](#reducing-warning-noise)
   * [Reporting bugs and wishes](#reporting-bugs-and-wishes)
   * [Authors](#authors)
   * [Contributing patches](#contributing-patches)

# Source Code

You can get clazy from:

- <https://github.com/KDE/clazy>
- git@git.kde.org:clazy
- git://anongit.kde.org/clazy
- Source for the v1.1-msvc2015 binary package: <http://download.kde.org/stable/clazy/1.1/clazy_v1.1-src.zip.mirrorlist>

# Build Instructions
## Linux

### Install dependencies
- OpenSUSE tumbleweed: `zypper install cmake git-core llvm llvm-devel llvm-clang llvm-clang-devel`
- Ubuntu-16.04: `apt-get install g++ cmake clang llvm git-core libclang-3.8-dev qtbase5-dev`
- Archlinux: `pacman -S make llvm clang python2 cmake qt5-base git gcc`
- Fedora: be sure to *remove* the llvm-static package and only install the one with dynamic libraries
- Other distros: Check llvm/clang build docs.

### Build and install clang
clang and LLVM >= 3.8 are required.
Use clazy v1.1 if you need 3.7 support.

If your distro provides clang then you can skip this step.


```
  $ git clone https://github.com/llvm-mirror/llvm.git <some_directory>
  $ cd <some_directory>/tools/ && git clone https://github.com/llvm-mirror/clang.git
  $ cd <some_directory>/projects && git clone https://github.com/llvm-mirror/compiler-rt.git
  $ mkdir <some_directory>/build && cd <some_directory>/build
  $ cmake -DCMAKE_INSTALL_PREFIX=<prefix> -DLLVM_TARGETS_TO_BUILD=X86 -DCMAKE_BUILD_TYPE=Release ..
  $ make -jX && make install
```

### Build clazy
```
  $ cd clazy/
  $ cmake -DCMAKE_INSTALL_PREFIX=<prefix> -DCMAKE_BUILD_TYPE=Release
  $ make && make install
```

See troubleshooting section if you have problems.

## Windows

### Pre-built msvc2015 clang and clazy binaries

The easiest way is to download the binaries from <http://download.kde.org/stable/clazy/1.1/clazy_v1.1-msvc2015.zip.mirrorlist>. Unzip it somewhere, add bin to PATH and you're ready to go. Use clazy-cl.bat as a drop-in replacement for cl.exe.

If you really want to build clang and clazy yourself then read on, otherwise skip the building topic.


### Build and install clang
These instructions assume your terminal is suitable for development (msvc2015).
jom, nmake, git, cmake and cl should be in your PATH.

clang and LLVM >= 4.0 are required.

Be sure to pass -DLLVM_EXPORT_SYMBOLS_FOR_PLUGINS=ON to CMake when building LLVM, otherwise clazy won't work.

```
  > git clone https://github.com/llvm-mirror/llvm.git <some_directory>
  > cd <some_directory>\tools\ && git clone https://github.com/llvm-mirror/clang.git
  > git checkout release_40
  > cd clang
  > git checkout release_40
  > mkdir <some_directory>\build && cd <some_directory>\build
  > cmake -DCMAKE_INSTALL_PREFIX=c:\my_install_folder\llvm\ -DLLVM_TARGETS_TO_BUILD="X86" -DLLVM_EXPORT_SYMBOLS_FOR_PLUGINS=ON -DCMAKE_BUILD_TYPE=Release -G "NMake Makefiles JOM" ..
  > jom
  > nmake install
  > Add c:\my_install_folder\llvm\bin\ to PATH
```

### Build clazy

Be sure to point CLANG_LIBRARY_IMPORT to clang.lib. It's probably inside your LLVM build dir since it doesn't get installed.

```
  > cd clazy\
  > cmake -DCMAKE_INSTALL_PREFIX=c:\my_install_folder\llvm\ -DCLANG_LIBRARY_IMPORT=C:\path\to\llvm-build\lib\clang.lib -DCMAKE_BUILD_TYPE=Release -G "NMake Makefiles JOM"
  > jom && nmake install
```

## macOS with MacPorts

### Install clang
```
$ sudo port install clang-3.9 llvm-3.9
$ sudo ln -sf /opt/local/bin/llvm-config-mp-3.9 /opt/local/bin/llvm-config
$ sudo port select --set clang mp-clang-3.9
```

### Build clazy
```
  $ export CXX=clang++
  $ cmake
  $ make
  $ make install
```

## macOS with Homebrew

### Install clang

```
$ brew install --with-clang llvm
```

### Build clazy
```
  $ export CXX=clang++
  $ export LLVM_ROOT=/usr/local/opt/llvm
  $ cmake
  $ make
  $ make install
```

# Setting up your project to build with clazy

Note: Wherever `clazy` it mentioned, replace with `clazy-cl.bat` if you're on Windows.
Note: If you prefer running clazy over a JSON compilation database instead of using it as a plugin, jump to [clazy-standalone](#clang-standalone-and-json-database-support).

You should now have the clazy command available to you, in `<prefix>/bin/`.
Compile your programs with it instead of clang++/g++.

Note that this command is just a convenience wrapper which calls:
`clang++ -Xclang -load -Xclang ClangLazy.so -Xclang -add-plugin -Xclang clang-lazy`

If you have multiple versions of clang installed (say clang++-3.8 and clang++-3.9)
you can choose which one to use by setting the CLANGXX environment variable, like so:
`export CLANGXX=clang++-3.8; clazy`

To build a CMake project use:
 `cmake . -DCMAKE_CXX_COMPILER=clazy`
and rebuild.

To make it the compiler for qmake projects, just run qmake like:
`qmake -spec linux-clang QMAKE_CXX="clazy"`

Alternatively, if you want to use clang directly, without the wrapper:
`qmake -spec linux-clang QMAKE_CXXFLAGS="-Xclang -load -Xclang ClangLazy.so -Xclang -add-plugin -Xclang clang-lazy"`

You can also edit mkspecs/common/clang.conf and change QMAKE_CXX to clazy instead of clang++ and run qmake -spec linux-clang

It's recommended that you disable pre-compiled headers and don't use ccache.

You're all set, clazy will now run some checks on your project, but not all of them.
Read on if you want to enable/disable which checks are run.

# List of checks

There are many checks and they are divided in levels:
- level0: Very stable checks, 99.99% safe, no false-positives
- level1: Similar to level0, but sometimes (rarely) there might be some false-positives
- level2: Sometimes has false-positives (20-30%).
- level3: Not always correct, possibly very noisy, might require a knowledgeable developer to review, might have a very big rate of false-positives, might have bugs.

clazy runs all checks from level1 by default.

- Checks from level0:
    - [connect-non-signal](src/checks/level0/README-connect-non-signal.md)
    - [container-anti-pattern](src/checks/level0/README-container-anti-pattern.md)
    - [lambda-in-connect](src/checks/level0/README-lambda-in-connect.md)
    - [mutable-container-key](src/checks/level0/README-mutable-container-key.md)
    - [qdatetime-utc](src/checks/level0/README-qdatetime-utc.md)    (fix-qdatetime-utc)
    - [qenums](src/checks/level0/README-qenums.md)
    - [qfileinfo-exists](src/checks/level0/README-qfileinfo-exists.md)
    - [qgetenv](src/checks/level0/README-qgetenv.md)    (fix-qgetenv)
    - [qmap-with-pointer-key](src/checks/level0/README-qmap-with-pointer-key.md)
    - [qstring-arg](src/checks/level0/README-qstring-arg.md)
    - [qstring-insensitive-allocation](src/checks/level0/README-qstring-insensitive-allocation.md)
    - [qstring-ref](src/checks/level0/README-qstring-ref.md)    (fix-missing-qstringref)
    - [qt-macros](src/checks/level0/README-qt-macros.md)
    - [qvariant-template-instantiation](src/checks/level0/README-qvariant-template-instantiation.md)
    - [temporary-iterator](src/checks/level0/README-temporary-iterator.md)
    - [unused-non-trivial-variable](src/checks/level0/README-unused-non-trivial-variable.md)
    - [writing-to-temporary](src/checks/level0/README-writing-to-temporary.md)
    - [wrong-qglobalstatic](src/checks/level0/README-wrong-qglobalstatic.md)

- Checks from level1:
    - [auto-unexpected-qstringbuilder](src/checks/level1/README-auto-unexpected-qstringbuilder.md)    (fix-auto-unexpected-qstringbuilder)
    - [child-event-qobject-cast](src/checks/level1/README-child-event-qobject-cast.md)
    - [detaching-temporary](src/checks/level1/README-detaching-temporary.md)
    - [foreach](src/checks/level1/README-foreach.md)
    - [incorrect-emit](src/checks/level1/README-incorrect-emit.md)
    - [inefficient-qlist-soft](src/checks/level1/README-inefficient-qlist-soft.md)
    - [missing-qobject-macro](src/checks/level1/README-missing-qobject-macro.md)
    - [non-pod-global-static](src/checks/level1/README-non-pod-global-static.md)
    - [post-event](src/checks/level1/README-post-event.md)
    - [qdeleteall](src/checks/level1/README-qdeleteall.md)
    - [qlatin1string-non-ascii](src/checks/level1/README-qlatin1string-non-ascii.md)
    - [qstring-left](src/checks/level1/README-qstring-left.md)
    - [range-loop](src/checks/level1/README-range-loop.md)
    - [returning-data-from-temporary](src/checks/level1/README-returning-data-from-temporary.md)
    - [rule-of-two-soft](src/checks/level1/README-rule-of-two-soft.md)

- Checks from level2:
    - [base-class-event](src/checks/level2/README-base-class-event.md)
    - [container-inside-loop](src/checks/level2/README-container-inside-loop.md)
    - [copyable-polymorphic](src/checks/level2/README-copyable-polymorphic.md)
    - [ctor-missing-parent-argument](src/checks/level2/README-ctor-missing-parent-argument.md)
    - [function-args-by-ref](src/checks/level2/README-function-args-by-ref.md)
    - [function-args-by-value](src/checks/level2/README-function-args-by-value.md)
    - [global-const-char-pointer](src/checks/level2/README-global-const-char-pointer.md)
    - [implicit-casts](src/checks/level2/README-implicit-casts.md)
    - [missing-typeinfo](src/checks/level2/README-missing-typeinfo.md)
    - [old-style-connect](src/checks/level2/README-old-style-connect.md)    (fix-old-style-connect)
    - [qstring-allocations](src/checks/level2/README-qstring-allocations.md)    (fix-qlatin1string-allocations,fix-fromLatin1_fromUtf8-allocations,fix-fromCharPtrAllocations)
    - [reserve-candidates](src/checks/level2/README-reserve-candidates.md)
    - [returning-void-expression](src/checks/level2/README-returning-void-expression.md)
    - [rule-of-three](src/checks/level2/README-rule-of-three.md)
    - [virtual-call-ctor](src/checks/level2/README-virtual-call-ctor.md)

- Checks from level3:
    - [assert-with-side-effects](src/checks/level3/README-assert-with-side-effects.md)
    - [bogus-dynamic-cast](src/checks/level3/README-bogus-dynamic-cast.md)
    - [detaching-member](src/checks/level3/README-detaching-member.md)

# Selecting which checks to enable

You may want to choose which checks to enable before starting to compile.
If you don't specify anything then all checks from level0 and level1 will run.
To specify a list of checks to run, or to choose a level, you can use the `CLAZY_CHECKS` env variable or pass arguments to the compiler.
You can disable checks by prefixing with `no-`, in case you don't want all checks from a given level.

## Example via env variable
```
export CLAZY_CHECKS="bogus-dynamic-cast,qmap-with-key-pointer,virtual-call-ctor" # Enables only these 3 checks
export CLAZY_CHECKS="level0,no-qenums" # Enables all checks from level0, except for qenums
export CLAZY_CHECKS="level0,detaching-temporary" # Enables all from level0 and also detaching-temporary
```
## Example via compiler argument
`clazy -Xclang -plugin-arg-clang-lazy -Xclang level0,detaching-temporary`
Don't forget to re-run cmake/qmake/etc if you altered the c++ flags to specify flags.

# clang-standalone and JSON database support

The `clazy-standalone` binary allows you to run clazy over a compilation database JSON file, in the same
way you would use `clang-tidy` or other clang tooling. This way you don't need to build your application,
only the static analysis is performed.

`clazy-standalone` supports the same env variables as the clazy plugin. You can also specify a list of checks via
the `-checks` argument.


Running on one cpp file:
`clazy-standalone -checks=install-event-filter,qmap-with-pointer-key,level0 -p compile_commands.json my.file.cpp`

Running on all cpp files:
`find . -name "*cpp" | xargs clazy-standalone -checks=level2 -p default/compile_commands.json`

See https://clang.llvm.org/docs/JSONCompilationDatabase.html for how to generate the compile_commands.json file. Basically it's generated
by passing `-DCMAKE_EXPORT_COMPILE_COMMANDS` to CMake, or using [Bear](https://github.com/rizsotto/Bear) to intercept compiler commands, or, if you're using `qbs`:

`qbs generate --generator clangdb`

Note: Be sure the clazy-standalone binary is located in the same folder as the clang binary. Otherwise it might have trouble
finding headers.

`clang-tidy` support will be added after <https://bugs.llvm.org//show_bug.cgi?id=32739> is fixed.

# Enabling Fixits

Some checks support fixits, in which clazy will re-write your source files whenever it can fix something.
You can enable a fixit through the env variable, for example:
`export CLAZY_FIXIT="fix-qlatin1string-allocations"`

Only one fixit can be enabled each time.
**WARNING**: Backup your code, don't blame me if a fixit is not applied correctly.
For better results don't use parallel builds, otherwise a fixit being applied in an header file might be done twice.

# Troubleshooting

- clang: symbol lookup error: `/usr/lib/x86_64-linux-gnu/ClangLazy.so: undefined symbol: _ZNK5clang15DeclarationName11getAsStringEv`.
  This is due to mixing ABIs. Your clang/llvm was compiled with the [new gcc c++ ABI][1] but you compiled the clazy plugin with clang (which uses [the old ABI][2]).

  The solution is to build the clazy plugin with gcc or use a distro which hasn't migrated to gcc5 ABI yet, such as archlinux.

[1]: https://gcc.gnu.org/onlinedocs/libstdc++/manual/using_dual_abi.html
[2]: https://llvm.org/bugs/show_bug.cgi?id=23529

- [Fedora] cmake can't find LLVM ?
  Try building llvm/clang yourself
  (There are reports that /usr/share/llvm/cmake/LLVM-Config.cmake is buggy).

- [Fedora] CommandLine Error: `Option 'opt-bisect-limit' registered more than once!`
  Remove the llvm-static package and use the dynamically linked libraries instead

- Some checks are mysteriously not producing warnings or not applying fixits ?
  Check if you have ccache interfering and turn it off.

- fatal error: 'stddef.h' file not found, while using `clazy-standalone`
  Be sure the clazy-standalone binary is located in the same folder as the clang binary.

- Be sure to disble pch.

- macOS: Be sure you're not using Apple Clang

# Qt4 compatibility mode

When running on codebases that must still compile with Qt4, you can pass `--qt4compat`
(a convenience option equivalent to passing `-Xclang -plugin-arg-clang-lazy -Xclang qt4-compat`)
to disable checks that only make sense with Qt5.

For example, to build a CMake project with Qt4 compatibility use:
 `CXX="clazy --qt4compat"; cmake .`
and rebuild.

# Reducing warning noise

If you think you found a false-positive, file a bug report.

If you want to suppress warnings from headers of Qt or 3rd party code, include them with `-isystem` instead of `-I`.

You can also suppress individual warnings by file or by line by inserting comments:

- To disable clazy in a specific source file, insert this comment, anywhere in the file:
`// clazy:skip`

- To disable specific checks in a source file, insert a comment such as
`// clazy:excludeall=check1,check2`

- To disable specific checks in specific source lines, insert a comment in the same line as the warning:
`(...) // clazy:exclude=check1,check2`

Don't include the `clazy-` prefix. If, for example, you want to disable qstring-allocations you would write:
`// clazy:exclude=qstring-allocations` not `clazy-qstring-allocations`.

# Reporting bugs and wishes

- bug tracker: <https://bugs.kde.org/enter_bug.cgi?product=clazy>
- IRC: #kde-clazy (freenode)
- E-mail: <smartins at kde.org>

# Authors

- Sérgio Martins

with contributions from:

- Allen Winter
- Kevin Funk
- Mathias Hasselmann
- Laurent Montel
- Albert Astals Cid
- Aurélien Gâteau
- Hannah von Reth
- Volker Krause
- Christian Ehrlicher

and thanks to:

- Klarälvdalens Datakonsult AB (<http://www.kdab.com>), for letting me work on clazy as a research project


# Contributing patches

<https://git.reviewboard.kde.org>
