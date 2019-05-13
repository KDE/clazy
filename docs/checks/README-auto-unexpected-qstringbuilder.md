# auto-unexpected-qstringbuilder

Finds places where auto is deduced to be `QStringBuilder` instead of `QString`, which introduces crashes.
Also warns for lambdas returning `QStringBuilder`.

#### Example

    #define QT_USE_QSTRINGBUILDER
    #include <QtCore/QString>
    (...)
    const auto path = "hello " +  QString::fromLatin1("world");
    qDebug() << path; // CRASH

#### Fixits

This check supports a fixit to rewrite your code. See the README.md on how to enable it.
