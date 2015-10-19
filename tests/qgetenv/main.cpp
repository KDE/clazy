#include <QtCore/qglobal.h>
#include <QtCore/qbytearray.h>

int test()
{

    qgetenv("Foo").isEmpty();
    bool b = qgetenv("Foo").isNull();
    QByteArray ba = qgetenv("Foo");
    int a = qgetenv("Foo").toInt(&b);
    return 0;
}
