#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QByteArray>

void test()
{
    QString s;
    if (s == 5000) {} // Warn
    if (s == 'a') {} // OK
    if (s == "ok") {} // OK
    if (s == s) {} // OK
}


void testCrash() {
    QByteArray ba = "this is a qbytearray";
    if (ba == QStringLiteral("windows_generic_MSG")) {} // BUG: 502458, we have a member function call here and thus only one parameter
}
