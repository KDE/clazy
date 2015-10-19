#include <QtCore/qglobal.h>
#include <QtCore/qbytearray.h>

int test()
{

    qgetenv("Foo").isEmpty();
    qgetenv("Foo").isNull();
    QByteArray b = qgetenv("Foo");
    return 0;
}
