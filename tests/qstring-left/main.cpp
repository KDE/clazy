#include <QtCore/QString>

void test()
{
    QString s;
    auto r = s.left(0); // Warning
    r = s.left(1); // Warning
    r = s.left(2); // OK
    r = s.left(100); // OK
    r = s.left(0 + 1); // OK
    int a = 1;
    r = s.left(a); // OK
}
