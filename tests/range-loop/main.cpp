#include <QtCore/QList>
#include <vector>
#include <QtCore/QMap>
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
    for (int i : qt_container) { // Warning
    }

    const QList<int> const_qt_container;
    for (int i : const_qt_container) { // OK
    }

    for (int i : getQtList()) { // Warning
    }

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
    for (int i : m) {
    }
}