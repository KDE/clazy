#include <QtCore/QList>
#include <vector>


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

}
