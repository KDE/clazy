# qt-keyword-emit
Warns when using Qt `emit` keyword.

This check is disabled by default and must be explicitly enabled.

Note that C++20 has a method named `emit()`, therefore it's advisable to switch to using the `Q_EMIT` macro to prevent name conflicts (perferably before starting to use Qt code with C++20). See https://cplusplus.github.io/LWG/issue3399 if you're interested in the details.

This check is mainly useful due to its *fixit* to automatically convert the `emit` keyword to its `Q_` variant. Once you've converted all usages, then simply guard against `emit` being reintroduced in your codebase with `ADD_DEFINITIONS(-DQT_NO_EMIT)` (CMake) or `CONFIG += no_keywords` (qmake).

This check is a "subset" of the [qt-keywords](docs/checks/README-qt-keywords.md) check, which warns about `emit` and other Qt keywords.
