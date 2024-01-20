#include <QtCore/QMap>
#include <QtCore/QHash>


QMultiMap<int,int> getMultiMap() { return {}; }
void testDerivedClass()
{
    getMultiMap().begin(); // Warning
    getMultiMap().insert(1, 1); // Warning
}

void maps()
{
    QMap<int, QStringList> map;
    map.value(0).first(); // OK for Qt5, value() returns const T
    map[0].removeAll("asd"); // OK
    map.values().first(); // OK, QMap::values() isn't shared
}

void test_ctor()
{
    QStringList().first();
    QByteArray key = "key";
    QByteArray(key + key).data(); // In Qt5, this is const
}
