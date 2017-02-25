clazy v1.1
===========

clazy is a compiler plugin which allows clang to understand Qt semantics. You get more than 50 Qt related compiler warnings, ranging from unneeded memory allocations to misusage of API, including fix-its for automatic refactoring.

Table of contents
=================

   * [Source Code](#source-code)
   * [Build Instructions (Linux)](#build-instructions-linux)
      * [Install Dependencies:](#install-dependencies)
      * [Build and install clang &gt;= 3.7 if your distro doesn't provide it:](#build-and-install-clang--37-if-your-distro-doesnt-provide-it)
      * [Build the clazy plugin:](#build-the-clazy-plugin)
   * [Build Instructions (Windows)](#build-instructions-windows)
      * [Build and install llvm and clang 4.0:](#build-and-install-llvm-and-clang-40)
      * [Build the clazy plugin:](#build-the-clazy-plugin-1)
   * [Build Instructions (macOS with MacPorts)](#build-instructions-macos-with-macports)
      * [Install clang and llvm from MacPorts](#install-clang-and-llvm-from-macports)
      * [Build the clazy plugin](#build-the-clazy-plugin-2)
   * [Build Instructions (macOS with Homebrew)](#build-instructions-macos-with-homebrew)
      * [Install clang and llvm from Homebrew](#install-clang-and-llvm-from-homebrew)
      * [Build the clazy plugin](#build-the-clazy-plugin-3)
   * [Setting up your project to build with clazy](#setting-up-your-project-to-build-with-clazy)
   * [Selecting which checks to enable](#selecting-which-checks-to-enable)
      * [Description of each level](#description-of-each-level)
      * [Example via env variable](#example-via-env-variable)
      * [Example via compiler argument](#example-via-compiler-argument)
   * [Enabling Fixits](#enabling-fixits)
   * [Troubleshooting](#troubleshooting)
   * [Reducing warning noise](#reducing-warning-noise)
   * [Reporting bugs and wishes](#reporting-bugs-and-wishes)
   * [Contributing patches](#contributing-patches)

# Source Code

You can get clazy from:

- <https://github.com/KDE/clazy>
- git@git.kde.org:clazy
- <http://anongit.kde.org/clazy>

# Build Instructions (Linux)

## Install Dependencies:
- OpenSUSE tumbleweed: `zypper install cmake git-core llvm llvm-devel llvm-clang llvm-clang-devel`
- Ubuntu-16.04: `apt-get install g++ cmake clang llvm git-core libclang-3.8-dev qtbase5-dev`
- Archlinux: `pacman -S make llvm clang python2 cmake qt5-base git gcc`
- Fedora: be sure to *remove* the llvm-static package and only install the one with dynamic libraries
- Other distros: Check llvm/clang build docs.

## Build and install clang >= 3.7 if your distro doesn't provide it:
```
  $ git clone https://github.com/llvm-mirror/llvm.git <some_directory>
  $ cd <some_directory>/tools/ && git clone https://github.com/llvm-mirror/clang.git
  $ cd <some_directory>/projects && git clone https://github.com/llvm-mirror/compiler-rt.git
  $ mkdir <some_directory>/build && cd <some_directory>/build
  $ cmake -DCMAKE_INSTALL_PREFIX=<prefix> -DLLVM_TARGETS_TO_BUILD=X86 -DCMAKE_BUILD_TYPE=Release ..
  $ make -jX && make install
```

## Build the clazy plugin:
```
  $ cd clazy/
  $ cmake -DCMAKE_INSTALL_PREFIX=<prefix> -DCMAKE_BUILD_TYPE=Release
  $ make && make install
```

See troubleshooting section if you have problems.

# Build Instructions (Windows)

The instructions assume your terminal is suitable for development (msvc2015).
jom, nmake, git, cmake and cl should be in your PATH. Be aware that due to limitations on Windows
you'll have to make sure to always call clang.exe, never clang++.exe or clang-cl.exe.

## Build and install llvm and clang 4.0:

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

## Build the clazy plugin:

Be sure to point CLANG_LIBRARY_IMPORT to clang.lib. It's probably inside your LLVM build dir since it doesn't get installed.

```
  > cd clazy\
  > cmake -DCMAKE_INSTALL_PREFIX=c:\my_install_folder\llvm\ -DCLANG_LIBRARY_IMPORT=C:\path\to\llvm-build\lib\clang.lib -DCMAKE_BUILD_TYPE=Release -G "NMake Makefiles JOM"
  > jom && nmake install
```

# Build Instructions (macOS with MacPorts)

## Install clang and llvm from MacPorts

```
$ sudo port install clang-3.9 llvm-3.9
$ sudo ln -sf /opt/local/bin/llvm-config-mp-3.9 /opt/local/bin/llvm-config
$ sudo port select --set clang mp-clang-3.9
```

## Build the clazy plugin
```
  $ export CXX=clang++
  $ cmake
  $ make
  $ make install
```

# Build Instructions (macOS with Homebrew)

## Install clang and llvm from Homebrew

```
$ brew install --with-clang llvm
```

## Build the clazy plugin
```
  $ export CXX=clang++
  $ export LLVM_ROOT=/usr/local/opt/llvm
  $ cmake
  $ make
  $ make install
```

# Setting up your project to build with clazy

Note: Wherever `clazy` it mentioned, replace with `clazy-cl.bat` if you're on Windows.

You should now have the clazy command available to you, in `<prefix>/bin/`.
Compile your programs with it instead of clang++/g++.

Note that this command is just a convenience wrapper which calls:
`clang++ -Xclang -load -Xclang ClangLazy.so -Xclang -add-plugin -Xclang clang-lazy`

If you have multiple versions of clang installed (say clang++-3.7 and clang++-3.8)
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

# Selecting which checks to enable

You may want to choose which checks to enable before starting to compile.
There are many checks and they are divided in levels:


- Checks from level0:
    - connect-non-signal
    - container-anti-pattern
    - lambda-in-connect
    - mutable-container-key
    - qdatetime-utc    (fix-qdatetime-utc)
    - qenums
    - qfileinfo-exists
    - qgetenv    (fix-qgetenv)
    - qmap-with-pointer-key
    - qstring-arg
    - qstring-insensitive-allocation
    - qstring-ref    (fix-missing-qstringref)
    - qt-macros
    - qvariant-template-instantiation
    - temporary-iterator
    - unused-non-trivial-variable
    - writing-to-temporary
    - wrong-qglobalstatic

- Checks from level1:
    - auto-unexpected-qstringbuilder    (fix-auto-unexpected-qstringbuilder)
    - child-event-qobject-cast
    - detaching-temporary
    - foreach
    - incorrect-emit
    - inefficient-qlist-soft
    - missing-qobject-macro
    - non-pod-global-static
    - post-event
    - qdeleteall
    - qlatin1string-non-ascii
    - qstring-left
    - range-loop
    - returning-data-from-temporary
    - rule-of-two-soft

- Checks from level2:
    - base-class-event
    - container-inside-loop
    - copyable-polymorphic
    - ctor-missing-parent-argument
    - function-args-by-ref
    - function-args-by-value
    - global-const-char-pointer
    - implicit-casts
    - missing-typeinfo
    - old-style-connect    (fix-old-style-connect)
    - qstring-allocations    (fix-qlatin1string-allocations,fix-fromLatin1_fromUtf8-allocations,fix-fromCharPtrAllocations)
    - reserve-candidates
    - returning-void-expression
    - rule-of-three
    - virtual-call-ctor

- Checks from level3:
    - assert-with-side-effects
    - bogus-dynamic-cast
    - detaching-member

## Description of each level
- level0: Very stable checks, 99.99% safe, no false-positives
- level1: Similar to level0, but sometimes (rarely) there might be some false-positives
- level2: Sometimes has false-positives (20-30%).
- level3: Not always correct, possibly very noisy, might require a knowledgeable developer to review, might have a very big rate of false-positives, might have bugs.

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
- E-mail: <smartins@kde.org>

# Contributing patches

<https://git.reviewboard.kde.org>
