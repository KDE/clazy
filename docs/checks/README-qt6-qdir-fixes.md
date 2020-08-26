Warn against QDir deprecated API in Qt6

The code is fixed when possible.
Qdir dir; dir = "..." becomes	QDir dir; dir.setPath("...");
Qdir::addResourceSearchPath() becomes QDir::addSearchPath() with prefix.
Only warning are emitted for addResourceSearchPath.

This fix-it is intended to aid the user porting from Qt5 to Qt6.
Run this check with Qt5. The produced fixed code will compile on Qt6.