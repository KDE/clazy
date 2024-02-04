#include <QtCore/qglobal.h>
#include <QtCore/qbytearray.h>

int test()
{

    qgetenv("Foo").isEmpty();
    bool b = qgetenv("Foo").isNull();
    QByteArray ba = qgetenv("Foo");
    int ignoreOK = qgetenv("Foo").toInt();
    int checkOK = qgetenv("Foo").toInt(&b);
    int dontfix = qgetenv("Foo").toInt(&b, 16);
    return 0;
}
