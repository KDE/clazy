#define QT_USE_QSTRINGBUILDER
#include <QtCore/QString>

void test()
{
    const auto s1 = "tests/" + QString::fromLatin1("foo"); // Warning
    const auto s2 = "tests/" % QString::fromLatin1("foo"); // Warning
    const QString s3 = "tests/" + QString::fromLatin1("foo"); // OK
    const QString s4 = "tests/" % QString::fromLatin1("foo"); // OK

    [] {
        return "tests/" % QString::fromLatin1("foo"); // Warn
    };

    [] {
        return QString(); // OK
    };
}

