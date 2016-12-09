# qt-macros

Finds misusages of some Qt macros.

The two cases are:
- Using `Q_OS_WINDOWS` instead of `Q_OS_WIN` (The former doesn't exist).
- Testing a `Q_OS_XXX` macro before including `qglobal.h`
