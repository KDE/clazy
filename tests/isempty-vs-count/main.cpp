#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QMultiMap>

void test()
{
    QList<int> list;
    if (list.count()) {}   // Warning
    bool b1 = list.count(); // Warning
    bool b2 = list.size(); // Warning
    if (list.indexOf(1)) {} // OK

    QMap<int, int> map;
    if (map.count()) {} // Warning
    if (map.count(2)) {} // Warning

    QMultiMap<int, int> multiMap;
    if (multiMap.count()) {} // Warning
    if (multiMap.count(2)) {} // Warning
    if (multiMap.count(2, 3)) {} // Warning
}
