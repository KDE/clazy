#include <QtCore/QSet>
#include <QtCore/QHash>
#include <QtCore/QMap>
#include <QtCore/QList>
#include <QtCore/QVector>

void test()
{
    QSet<int> set;
    QMap<int,int> map;
    QHash<int,int> hash;
    QList<int> list;
    QVector<int> vec;

    vec.toList().count(); // Warning
    map.values()[0]; // Warning
    int a = hash.keys().at(0); // Warning
    a = map.keys().at(0); // Warning
    list.toVector().size(); // Warning
}

void testRangeLoop()
{
    QHash<int,int> hash;
    for (auto i : hash.values()) {} // Warning
    for (auto i : hash.keys()) {} // Warning
    for (auto i : hash) {} // OK
    foreach (auto i, hash.keys()) {} // Warning
    foreach (auto i, hash.values()) {} // Warning
    foreach (auto i, hash) {} // OK
}
