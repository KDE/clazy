# auto-unexpected-qstringbuilder

Finds places where auto is deduced to be `QStringBuilder` instead of `QString`, which introduces crashes.

#### Example

    #define QT_USE_QSTRINGBUILDER
    #include <QtCore/QString>
    (...)
    const auto path = "hello " +  QString::fromLatin1("world");
    qDebug() << path; // CRASH

#### Fixits

    export CLAZY_FIXIT="fix-auto-unexpected-qstringbuilder"
