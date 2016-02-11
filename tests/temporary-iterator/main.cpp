#include <QtCore/QList>
#include <QtCore/QVariant>
#include <vector>
#include <QtCore/QHash>
Q_GLOBAL_STATIC(QList<int>, s_list);

QList<int> getList()
{
  QList<int> list;
  list << 1;
  return list;
}

QList<int>& getListRef()
{
  static QList<int> list;
  return list;
}

void test()
{
    for (auto it = getList().begin(); it != getList().end(); ++it) {} // Warning
    for (auto it = getListRef().begin(); it != getListRef().end(); ++it) {} // OK

    QList<int> localList;
    for (auto it = localList.begin(); it != localList.end(); ++it) {} // OK
    localList.cbegin(); // OK
    getListRef().cbegin(); // OK
    getList().cbegin(); // Warning

    s_list->cbegin(); // OK
    QVariant variant;
    variant.toList().cbegin(); // OK
}

void acceptsInt(int) {}

void testDereference()
{
    int value = *getList().cbegin(); // OK
    value = *(getList().cbegin()); // OK
    acceptsInt(*getList().cbegin()); //OK
    const QHash<int, QList<int> > listOfLists;
    int ns = 1;
    for (auto it = listOfLists[ns].constBegin(); it != listOfLists[ns].constEnd(); ++it) {} // OK

}

QHash<int,int> getHash()
{
    return {};
}

void testMember()
{
    getHash().cbegin(); // Warning
    getHash().cbegin().value(); // OK
}
