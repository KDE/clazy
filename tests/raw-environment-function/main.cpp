#define _CRT_SECURE_NO_WARNINGS // Silence warnings with MSVC
#include <QtCore/QObject>
#include <QtCore/QString>
#include <stdlib.h>

void test()
{
    QByteArray array = qgetenv("foo"); // OK
    qputenv("foo", "bar"); // OK

    const char *array2 = getenv("foo"); // Warn
    char *var;
    int i = putenv(var); // Warn

    const char *array3 = ::getenv("foo"); // Warn
}
