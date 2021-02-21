#include <QtCore/QList>
#include <vector>
#include <QtCore/QMap>
#include <QtCore/QString>

void receivingList(QList<int>);
void receivingMap(QMultiMap<int,int>);

using namespace std;

QList<int> getQtList()
{
    return {}; // dummy, not important
}

const QList<int> getConstQtList()
{
    return {}; // dummy, not important
}

const QList<int> & getConstRefQtList()
{
    static QList<int> foo;
    return foo;
}

void testQtContainer()
{
    QList<int> qt_container;
    receivingList(qt_container);
    for (int i : qt_container) { // Warning
    }

    const QList<int> const_qt_container;
    for (int i : const_qt_container) { // OK
    }

    for (int i : getQtList()) { // Warning
    }

    for (int i : qt_container) { }  // Warning
    for (const int &i : qt_container) { } // Warning
    for (int &i : qt_container) { } // OK




    for (int i : getConstQtList()) { // OK
    }

    for (int i : getConstRefQtList()) { // OK
    }

    vector<int> stl_container;
    for (int i : stl_container) { // OK
    }
}

class A {
public:
    void foo()
    {
        for (int a : m_stlVec) {} // OK
    }

    std::vector<int> m_stlVec;
};

void testQMultiMapDetach()
{
    QMultiMap<int,int> m;
    receivingMap(m);
    for (int i : m) {
    }
}

void testBug367485()
{
    QList<int> list;
    for (auto a : list) {} // OK

    QList<int> list2;
    receivingList(list2);
    for (auto a : list2) {} // Warning

    QList<int> list3;
    for (auto a : list3) {} // OK
    receivingList(list3);

    QList<int> list4;
    foreach (auto a, list4) {} // OK
    receivingList(list4);
}

struct SomeStruct
{
    QStringList m_list;
    void test_add_qasconst_fixits()
    {
         for (const auto &s : m_list) {} // Warn
    }

    QStringList getList();
};


void test_add_qasconst_fixits()
{
    SomeStruct f;
    for (const auto &s : f.m_list) {}  // Warn

    SomeStruct *f2;
    for (const auto &s : f2->m_list) {}  // Warn

    QStringList locallist = f.getList();
    for (const auto &s : locallist) {} // Warn

    for (const auto &s : getQtList()) {} // Warn

    for (const auto &s : f.getList()) {} // Warn
}
