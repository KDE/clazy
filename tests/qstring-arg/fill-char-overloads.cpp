#include <QtCore/QString>

void test()
{
    int i = 1, latitude;
    QString s = QString().arg("foo", i); // Warning
    s = s.arg(s, i, 3); // Warning
    s = s.arg(1, latitude); // Warning
    s = s.arg(s); // OK
    s = s.arg(s, 1); // OK
}
