# qt4-qstring-from-array

Warns when using `QString` methods taking a `char*` or a `QByteArray`.
These are dangerous in Qt4 because Qt4 assumes they are in latin1, while Qt5 assumes they are in utf8.
