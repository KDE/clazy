#include <QtCore/QList>
#include <vector>

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
