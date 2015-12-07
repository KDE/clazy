#include <QtCore/QList>

void test()
{
    QList<int> list;
    if (list.count()) {}   // Warning
    bool b1 = list.count(); // Warning
    bool b2 = list.size(); // Warning
    if (list.indexOf(1)) {} // OK
}
