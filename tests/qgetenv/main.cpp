#include <QtCore/qglobal.h>
#include <QtCore/qbytearray.h>

int test()
{

    qgetenv("Foo").isEmpty();
    bool b = qgetenv("Foo").isNull();
    QByteArray ba = qgetenv("Foo");
    int fixmeWarnAboutBaseChange = qgetenv("Foo").toInt();
    int fixmeWarnAboutBaseChangeAndKeepOk = qgetenv("Foo").toInt(&b);
    int dontfix = qgetenv("Foo").toInt(&b, 16);
    int fixmeWithoutChangingInternalBase = qgetenv("Foo").toInt(&b, 0);
    int fixmeWithoutChangingInternalBaseAndIgnoringOk = qgetenv("Foo").toInt(nullptr, 0);
    return 0;
}
