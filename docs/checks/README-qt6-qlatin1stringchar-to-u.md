# qt6-qlatin1stringchar-to-u

> Note: This check is deprecated with Clazy 1.16 and will be removed in later versions!

Warns when using `QLatin1String` and replaces it with `u`.

This fix-it is intended to aid the user porting from Qt5 to Qt6.
Run this check with Qt5. The produced fixed code will compile on Qt6.
