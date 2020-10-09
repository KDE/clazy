*WARNING:* master is the development branch. Please use the v1.7 branch.

clazy v1.8
===========

clazy is a compiler plugin which allows clang to understand Qt semantics. You get more than 50 Qt related compiler warnings, ranging from unneeded memory allocations to misusage of API, including fix-its for automatic refactoring.

Table of contents
=================

   * [Source Code](#source-code)
   * [Pre-built binaries](#pre-built-binaries)
   * [Build Instructions](#build-instructions)
      * [Linux](#linux)
         * [Install dependencies](#install-dependencies)
         * [Build and install clang](#build-and-install-clang)
         * [Build clazy](#build-clazy)
      * [Windows](#windows)
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
   * [clazy-standalone and JSON database support](#clazy-standalone-and-json-database-support)
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
- <https://invent.kde.org/sdk/clazy>

# Pre-built binaries

Pre-built clazy-1.7 binaries based on clang-10 for MSVC2019 and Linux AppImage are produced by KDAB, you can get them from https://downloads.kdab.com/clazy/.

# Build Instructions
## Linux

### Install dependencies
- OpenSUSE tumbleweed: `zypper install cmake git-core llvm llvm-devel llvm-clang llvm-clang-devel`
- Ubuntu: `apt install g++ cmake clang llvm-dev git-core libclang-dev`
- Archlinux: `pacman -S make llvm clang python2 cmake git gcc`
- Fedora: be sure to *remove* the llvm-static package and only install the one with dynamic libraries
- Other distros: Check llvm/clang build docs.

### Build and install clang
clang and LLVM >= 5.0 are required.

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

### Build and install clang
These instructions assume your terminal is suitable for development.
jom and nmake (or ninja), git, cmake, and cl (msvc2015 or later) should be in your PATH.

clang and LLVM >= 5.0 are required.

Be sure to pass -DLLVM_EXPORT_SYMBOLS_FOR_PLUGINS=ON to CMake when building LLVM, otherwise clazy won't work.

```
  > git clone https://github.com/llvm-mirror/llvm.git <some_directory>
  > cd <some_directory>\tools\ && git clone https://github.com/llvm-mirror/clang.git
  > git checkout release_90
  > cd clang
  > git checkout release_90
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
$ sudo port install llvm-8.0 clang-8.0 cmake ninja coreutils
$ sudo ln -sf /opt/local/bin/llvm-config-mp-8.0 /opt/local/bin/llvm-config
$ sudo port select --set clang mp-clang-8.0
```

### Build clazy
```
  $ export CXX=clang++
  $ cmake
  $ make
  $ make install
```

## macOS with Homebrew

The recommended way is to build clazy yourself, but alternatively you can try user recipes, such as:

```
$ brew install kde-mac/kde/clazy
```

for stable branch, or for master:

```
$ brew install kde-mac/kde/clazy --HEAD
```

As these are not verified or tested by the clazy developers please don't report bugs to us.

For building yourself, read on. You'll have to install clang and build clazy from source.

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

Note: Wherever `clazy` is mentioned, replace with `clazy-cl.bat` if you're on Windows, or replace with `Clazy-x86_64.AppImage` if you're using AppImage.
Note: If you prefer running clazy over a JSON compilation database instead of using it as a plugin, jump to [clazy-standalone](#clazy-standalone-and-json-database-support).

You should now have the clazy command available to you, in `<prefix>/bin/`.
Compile your programs with it instead of clang++/g++.

Note that this command is just a convenience wrapper which calls:
`clang++ -Xclang -load -Xclang ClazyPlugin.so -Xclang -add-plugin -Xclang clazy`

If you have multiple versions of clang installed (say clang++-3.8 and clang++-3.9)
you can choose which one to use by setting the CLANGXX environment variable, like so:
`export CLANGXX=clang++-3.8; clazy`

To build a CMake project use:
 `cmake . -DCMAKE_CXX_COMPILER=clazy`
and rebuild.

To make it the compiler for qmake projects, just run qmake like:
`qmake -spec linux-clang QMAKE_CXX="clazy"`

On Windows with MSVC it's simply: `qmake QMAKE_CXX="clazy-cl.bat"`

Alternatively, if you want to use clang directly, without the wrapper:
`qmake -spec linux-clang QMAKE_CXXFLAGS="-Xclang -load -Xclang ClazyPlugin.so -Xclang -add-plugin -Xclang clazy"`

On Windows it's similar, just inspect the contents of `clazy-cl.bat`.


It's recommended that you disable pre-compiled headers and don't use ccache.

You're all set, clazy will now run some checks on your project, but not all of them.
Read on if you want to enable/disable which checks are run.

# List of checks

There are many checks and they are divided in levels:
- level0: Very stable checks, 99.99% safe, mostly no false-positives, very desirable
- level1: The default level. Very similar to level 0, slightly more false-positives but very few.
- level2: Also very few false-positives, but contains noisy checks which not everyone agree should be default.
- manual: Checks here need to be enabled explicitly, as they don't belong to any level. They can be very stable or very unstable.

clazy runs all checks from level1 by default.

- Checks from Manual Level:
    - [assert-with-side-effects](docs/checks/README-assert-with-side-effects.md)
    - [container-inside-loop](docs/checks/README-container-inside-loop.md)
    - [detaching-member](docs/checks/README-detaching-member.md)
    - [heap-allocated-small-trivial-type](docs/checks/README-heap-allocated-small-trivial-type.md)
    - [ifndef-define-typo](docs/checks/README-ifndef-define-typo.md)
    - [inefficient-qlist](docs/checks/README-inefficient-qlist.md)
    - [isempty-vs-count](docs/checks/README-isempty-vs-count.md)
    - [jni-signatures](docs/checks/README-jni-signatures.md)
    - [qhash-with-char-pointer-key](docs/checks/README-qhash-with-char-pointer-key.md)
    - [qproperty-type-mismatch](docs/checks/README-qproperty-type-mismatch.md)
    - [qrequiredresult-candidates](docs/checks/README-qrequiredresult-candidates.md)
    - [qstring-varargs](docs/checks/README-qstring-varargs.md)
    - [qt-keywords](docs/checks/README-qt-keywords.md)    (fix-qt-keywords)
    - [qt4-qstring-from-array](docs/checks/README-qt4-qstring-from-array.md)    (fix-qt4-qstring-from-array)
    - [qt6-header-fixes](docs/checks/README-qt6-header-fixes.md)    (fix-qt6-header-fixes)
    - [qt6-qdir-fixes](docs/checks/README-qt6-qdir-fixes.md)    (fix-qt6-qdir-fixes)
    - [qt6-qhash-signature](docs/checks/README-qt6-qhash-signature.md)    (fix-qt6-qhash-signature)
    - [qt6-qlatin1stringchar-to-u](docs/checks/README-qt6-qlatin1stringchar-to-u.md)    (fix-qt6-qlatin1stringchar-to-u)
    - [qvariant-template-instantiation](docs/checks/README-qvariant-template-instantiation.md)
    - [raw-environment-function](docs/checks/README-raw-environment-function.md)
    - [reserve-candidates](docs/checks/README-reserve-candidates.md)
    - [signal-with-return-value](docs/checks/README-signal-with-return-value.md)
    - [thread-with-slots](docs/checks/README-thread-with-slots.md)
    - [tr-non-literal](docs/checks/README-tr-non-literal.md)
    - [unneeded-cast](docs/checks/README-unneeded-cast.md)
    - [use-chrono-in-qtimer](docs/checks/README-use-chrono-in-qtimer.md)

- Checks from Level 0:
    - [connect-by-name](docs/checks/README-connect-by-name.md)
    - [connect-non-signal](docs/checks/README-connect-non-signal.md)
    - [connect-not-normalized](docs/checks/README-connect-not-normalized.md)
    - [container-anti-pattern](docs/checks/README-container-anti-pattern.md)
    - [empty-qstringliteral](docs/checks/README-empty-qstringliteral.md)
    - [fully-qualified-moc-types](docs/checks/README-fully-qualified-moc-types.md)
    - [lambda-in-connect](docs/checks/README-lambda-in-connect.md)
    - [lambda-unique-connection](docs/checks/README-lambda-unique-connection.md)
    - [lowercase-qml-type-name](docs/checks/README-lowercase-qml-type-name.md)
    - [mutable-container-key](docs/checks/README-mutable-container-key.md)
    - [overloaded-signal](docs/checks/README-overloaded-signal.md)
    - [qcolor-from-literal](docs/checks/README-qcolor-from-literal.md)
    - [qdatetime-utc](docs/checks/README-qdatetime-utc.md)    (fix-qdatetime-utc)
    - [qenums](docs/checks/README-qenums.md)
    - [qfileinfo-exists](docs/checks/README-qfileinfo-exists.md)
    - [qgetenv](docs/checks/README-qgetenv.md)    (fix-qgetenv)
    - [qmap-with-pointer-key](docs/checks/README-qmap-with-pointer-key.md)
    - [qstring-arg](docs/checks/README-qstring-arg.md)
    - [qstring-comparison-to-implicit-char](docs/checks/README-qstring-comparison-to-implicit-char.md)
    - [qstring-insensitive-allocation](docs/checks/README-qstring-insensitive-allocation.md)
    - [qstring-ref](docs/checks/README-qstring-ref.md)    (fix-missing-qstringref)
    - [qt-macros](docs/checks/README-qt-macros.md)
    - [strict-iterators](docs/checks/README-strict-iterators.md)
    - [temporary-iterator](docs/checks/README-temporary-iterator.md)
    - [unused-non-trivial-variable](docs/checks/README-unused-non-trivial-variable.md)
    - [writing-to-temporary](docs/checks/README-writing-to-temporary.md)
    - [wrong-qevent-cast](docs/checks/README-wrong-qevent-cast.md)
    - [wrong-qglobalstatic](docs/checks/README-wrong-qglobalstatic.md)

- Checks from Level 1:
    - [auto-unexpected-qstringbuilder](docs/checks/README-auto-unexpected-qstringbuilder.md)    (fix-auto-unexpected-qstringbuilder)
    - [child-event-qobject-cast](docs/checks/README-child-event-qobject-cast.md)
    - [connect-3arg-lambda](docs/checks/README-connect-3arg-lambda.md)
    - [const-signal-or-slot](docs/checks/README-const-signal-or-slot.md)
    - [detaching-temporary](docs/checks/README-detaching-temporary.md)
    - [foreach](docs/checks/README-foreach.md)
    - [incorrect-emit](docs/checks/README-incorrect-emit.md)
    - [inefficient-qlist-soft](docs/checks/README-inefficient-qlist-soft.md)
    - [install-event-filter](docs/checks/README-install-event-filter.md)
    - [non-pod-global-static](docs/checks/README-non-pod-global-static.md)
    - [overridden-signal](docs/checks/README-overridden-signal.md)
    - [post-event](docs/checks/README-post-event.md)
    - [qdeleteall](docs/checks/README-qdeleteall.md)
    - [qhash-namespace](docs/checks/README-qhash-namespace.md)
    - [qlatin1string-non-ascii](docs/checks/README-qlatin1string-non-ascii.md)
    - [qproperty-without-notify](docs/checks/README-qproperty-without-notify.md)
    - [qstring-left](docs/checks/README-qstring-left.md)
    - [range-loop](docs/checks/README-range-loop.md)    (fix-range-loop-add-ref,fix-range-loop-add-qasconst)
    - [returning-data-from-temporary](docs/checks/README-returning-data-from-temporary.md)
    - [rule-of-two-soft](docs/checks/README-rule-of-two-soft.md)
    - [skipped-base-method](docs/checks/README-skipped-base-method.md)
    - [virtual-signal](docs/checks/README-virtual-signal.md)

- Checks from Level 2:
    - [base-class-event](docs/checks/README-base-class-event.md)
    - [copyable-polymorphic](docs/checks/README-copyable-polymorphic.md)
    - [ctor-missing-parent-argument](docs/checks/README-ctor-missing-parent-argument.md)
    - [function-args-by-ref](docs/checks/README-function-args-by-ref.md)    (fix-function-args-by-ref)
    - [function-args-by-value](docs/checks/README-function-args-by-value.md)
    - [global-const-char-pointer](docs/checks/README-global-const-char-pointer.md)
    - [implicit-casts](docs/checks/README-implicit-casts.md)
    - [missing-qobject-macro](docs/checks/README-missing-qobject-macro.md)    (fix-missing-qobject-macro)
    - [missing-typeinfo](docs/checks/README-missing-typeinfo.md)
    - [old-style-connect](docs/checks/README-old-style-connect.md)    (fix-old-style-connect)
    - [qstring-allocations](docs/checks/README-qstring-allocations.md)    (fix-qlatin1string-allocations,fix-fromLatin1_fromUtf8-allocations,fix-fromCharPtrAllocations)
    - [returning-void-expression](docs/checks/README-returning-void-expression.md)
    - [rule-of-three](docs/checks/README-rule-of-three.md)
    - [static-pmf](docs/checks/README-static-pmf.md)
    - [virtual-call-ctor](docs/checks/README-virtual-call-ctor.md)

# Selecting which checks to enable

You may want to choose which checks to enable before starting to compile.
If you don't specify anything then all checks from level0 and level1 will run.
To specify a list of checks to run, or to choose a level, you can use the `CLAZY_CHECKS` env variable or pass arguments to the compiler.
You can disable checks by prefixing with `no-`, in case you don't want all checks from a given level.

## Example via env variable
```
export CLAZY_CHECKS="unneeded-cast,qmap-with-pointer-key,virtual-call-ctor" # Enables only these 3 checks
export CLAZY_CHECKS="level0,no-qenums" # Enables all checks from level0, except for qenums
export CLAZY_CHECKS="level0,detaching-temporary" # Enables all from level0 and also detaching-temporary
```
## Example via compiler argument
`clazy -Xclang -plugin-arg-clazy -Xclang level0,detaching-temporary`
Don't forget to re-run cmake/qmake/etc if you altered the c++ flags to specify flags.

# clazy-standalone and JSON database support

The `clazy-standalone` binary allows you to run clazy over a compilation database JSON file, in the same
way you would use `clang-tidy` or other clang tooling. This way you don't need to build your application,
only the static analysis is performed.

Note: If you're using the AppImage, use `Clazy-x86_64.AppImage --standalone` instead of `clazy-standalone`.

`clazy-standalone` supports the same env variables as the clazy plugin. You can also specify a list of checks via
the `-checks` argument.


Running on one cpp file:
`clazy-standalone -checks=install-event-filter,qmap-with-pointer-key,level0 -p compile_commands.json my.file.cpp`

Running on all cpp files:
`find . -name "*cpp" | xargs clazy-standalone -checks=level2 -p default/compile_commands.json`

See https://clang.llvm.org/docs/JSONCompilationDatabase.html for how to generate the compile_commands.json file. Basically it's generated
by passing `-DCMAKE_EXPORT_COMPILE_COMMANDS` to CMake, or using [Bear](https://github.com/rizsotto/Bear) to intercept compiler commands, or, if you're using `qbs`:

`qbs generate --generator clangdb`

**Note:** Be sure the clazy-standalone binary is located in the same folder as the clang binary, otherwise it will have trouble
finding builtin headers, like stddef.h. Alternatively, you can symlink to the folder containing the builtin headers:

(Assuming clazy was built with `-DCMAKE_INSTALL_PREFIX=/myprefix/`)

```
$ touch foo.c && clang++ '-###' -c foo.c 2>&1 | tr ' ' '\n' | grep -A1 resource # Make sure this clang here is not Apple clang. Use for example clang++-mp-8.0 if on macOS and haven't run `port select` yet.
  "-resource-dir"
  "/opt/local/libexec/llvm-8.0/lib/clang/8.0.1" # The interesting part is /opt/local/libexec/llvm-8.0
$ ln -sf /opt/local/libexec/llvm-8.0/lib/clang/ /myprefix/lib/clang
$ mkdir /myprefix/include/
$ ln -sf /opt/local/libexec/llvm-8.0/include/c++/ /myprefix/include/c++ # Required on macOS
```

If that doesn't work, run `clang -v` and check what's the InstalledDir. Move clazy-standalone to that folder.

`clang-tidy` support will be added after <https://bugs.llvm.org//show_bug.cgi?id=32739> is fixed.

# Enabling Fixits

Some checks support fixits, in which clazy will help re-write your source files whenever it can fix something.
Simply pass `-Xclang -plugin-arg-clazy -Xclang export-fixes` to clang, or `-export-fixes=somefile.yaml` for `clazy-standalone`.
Alternatively, set the `CLAZY_EXPORT_FIXES` env variable (works only with the plugin, not with standalone).
Then run `clang-apply-replacements <folder_with_yaml_files>`, which will modify your code.

When using fixits, prefer to run only a single check each time, so they don't conflict
with each other modifying the same source lines.

**WARNING**: Backup your code and make sure all changes done by clazy are correct.

# Troubleshooting

- clang: symbol lookup error: `/usr/lib/x86_64-linux-gnu/ClazyPlugin.so: undefined symbol: _ZNK5clang15DeclarationName11getAsStringEv`.
  This is due to mixing ABIs. Your clang/llvm was compiled with the [new gcc c++ ABI][1] but you compiled the clazy plugin with clang (which uses [the old ABI][2]).

  The solution is to build the clazy plugin with gcc or use a distro which hasn't migrated to gcc5 ABI yet, such as archlinux.

[1]: https://gcc.gnu.org/onlinedocs/libstdc++/manual/using_dual_abi.html
[2]: https://llvm.org/bugs/show_bug.cgi?id=23529

- [Fedora] cmake can't find LLVM ?
  Try building llvm/clang yourself
  (There are reports that /usr/share/llvm/cmake/LLVM-Config.cmake is buggy).

- [Fedora] CommandLine Error: `Option 'opt-bisect-limit' registered more than once!`
  Remove the llvm-static package and use the dynamically linked libraries instead.
  Alternatively, if you want to use llvm-static, see next item.

- CommandLine Error: `Option 'foo' registered more than once!`
  Means you're building against a static version of LLVM (*.a files instead of *.so).
  Try passing to cmake -DLINK_CLAZY_TO_LLVM=OFF when building clazy, this was tested
  successfully against a static LLVM 7.0, and might work with other versions.

- Some checks are mysteriously not producing warnings or not applying fixits ?
  Check if you have ccache interfering and turn it off.

- fatal error: 'stddef.h' file not found, while using `clazy-standalone`
  Be sure the clazy-standalone binary is located in the same folder as the clang binary.

- Be sure to disable pch.

- macOS: Be sure you're not using Apple Clang

- macOS: System Integrity Protection blocks the use of DYLD_LIBRARY_PATH. With SIP enabled you need to pass the full path
  to ClazyPlugin.dylib, otherwise you'll get `image not found` error.

- Windows: fatal error LNK1112: module machine type ‘X86’ conflicts with target machine type ‘x64’
  If you're building in 32-bit, open clazy-cl.bat and insert a -m32 argument.
  Should read: %~dp0\clang\clang.exe –driver-mode=cl -m32 (...)

# Qt4 compatibility mode

When running on codebases that must still compile with Qt4, you can pass `--qt4compat`
(a convenience option equivalent to passing `-Xclang -plugin-arg-clazy -Xclang qt4-compat`)
to disable checks that only make sense with Qt5.

For example, to build a CMake project with Qt4 compatibility use:
 `CXX="clazy --qt4compat"; cmake .`
and rebuild.

# Reducing warning noise

If you think you found a false-positive, file a bug report. But do make sure to test first without icecc/distcc enabled.

If you want to suppress warnings from headers of Qt or 3rd party code, include them with `-isystem` instead of `-I` (gcc/clang only).
For MSVC use `/external`, which is available since VS 15.6.

Alternatively you can set the CLAZY_HEADER_FILTER env variable to a regexp matching the path where you want warnings,
for example `CLAZY_HEADER_FILTER=.*myapplication.*`.

You can also exclude paths using a regexp by setting CLAZY_IGNORE_DIRS, for example `CLAZY_IGNORE_DIRS=.*my_qt_folder.*`.

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
- Christian Gagneraud
- Nikolai Kosjar
- Jesper K. Pedersen

and thanks to:

- Klarälvdalens Datakonsult AB (<http://www.kdab.com>), for letting me work on clazy as a research project


# Contributing patches

New features go to master and bug fixes go to the 1.7 branch.
The prefered way to contributing is by using KDE's GitLab instance,
see <https://community.kde.org/Infrastructure/GitLab>.

If you rather just create a pull request in https://github.com/KDE/clazy for a drive-by change, it's also fine, but beware that
the maintainer might forget to check on github and the KDE bot will close the PR. In that case just send a reminder to the maintainer (smartins at kde.org).
