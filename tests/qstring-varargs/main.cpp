#include <QtCore/QString>

void simple_printf(const char* ...)
{
}


void test()
{
    QString s;
    printf("%s", s); // Warn
    simple_printf("%s", s); // Warn
    const char *foo = "f";
    printf("%s", foo);

    QByteArray b;
    printf("%s", b); // Warn
}
