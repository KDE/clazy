#include <QtCore/QObject>
#include <QtCore/QString>

void test()
{
    QString s;
    if (s == 5000) {} // Warn
    if (s == 'a') {} // OK
    if (s == "ok") {} // OK
    if (s == s) {} // OK
}
