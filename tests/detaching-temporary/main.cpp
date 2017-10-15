#include <QtCore/QList>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QMap>
#include <QtCore/QFile>
#include <QtCore/QHash>
#include <QtCore/QSet>

QList<int> getList()
{
    return QList<int>();
}

void detach1()
{
    getList().first(); // Warning
}

void detach2()
{
    getList().at(0); // OK
}

void lvalue()
{
    QStringList s;
    s.first(); // OK
}

QStringList test_string() { return {}; }
QStringList& test_string_ref() { static QStringList s; return s; }
QStringList * test_string_ptr() { return {}; }
const QStringList test_const_string() { return {}; }
const QStringList & test_const_string_ref() { static QStringList s; return s; }
const QStringList * test_const_string_ptr() { return {}; }

void qstrings()
{
    QString s;
    s.toLatin1().data(); // OK, list isn't shared
    test_string().first(); // Warning
    test_const_string().first(); // OK
    test_const_string_ref().first(); // OK
    test_const_string_ptr()->first(); // OK

    test_string().first(); // Warning
    test_string_ref().first(); // Warning
    test_string_ptr()->first(); // OK
}


void maps()
{
    QMap<int, QStringList> map;
    map.value(0).first(); // OK, value() returns const T
    map[0].removeAll("asd"); // OK
    map.values().first(); // OK, QMap::values() isn't shared
}

void more()
{
    QFile::encodeName("foo").data();
}

void foo(QStringList *list)
{
    auto it = list->begin();
}

typedef QMap<int, QStringList> StringMap;
Q_GLOBAL_STATIC(StringMap, sISOMap)
void test_global_static()
{
    sISOMap()->insert(1, QStringList());
    sISOMap->insert(1, QStringList());
}

void test_ctor()
{
    QStringList().first();
    QByteArray key = "key";
    QByteArray(key + key).data();
}

struct TestThis : public QList<int>
{
    void foo()
    {
        begin();
    }
};

class Foo
{
public:
    QStringList list;
};

Foo * getFoo() { return new Foo(); }
Foo getFoo2() { return Foo(); }

void testThroughPointer()
{
    Foo *f;
    f->list.first(); // OK
    getFoo()->list.first(); // OK
    getFoo2().list.first(); // OK
}

QMap<int,int> getMap() { return {}; };
void testWriteMethod()
{
    getMap().insert(1,1); // Warning
    int a = getMap().take(1); // Warning
}

void testSubscriptOperator()
{
    getList()[0]; // Warning
    if (test_string()[0].isEmpty()) {} // Warning
    getMap()[0]; // Warning
    auto m = getMap();
    m[0];
}

typedef QList<int> IntList;
IntList getIntList() { return {}; }
void testTypedef()
{
    getIntList()[0]; // Warning
}

QStringList getStringList() { return {}; }
QMultiMap<int,int> getMultiMap() { return {}; }
void testDerivedClass()
{
    getStringList().first(); // Warning
    getStringList()[0]; // Warning
    getMultiMap().begin(); // Warning
    getMultiMap().insert(1, 1); // Warning
}

void testQStringListSpecificMethods()
{
    getStringList().removeDuplicates();
}

QString getString() { return ""; }
void testQString()
{
    getString().begin(); // Warning
    getString()[1]; // Warning
}

extern QHash<QString, int> &initializers();
QString getStr() { return QString(); }

static auto bar = [] {
        return initializers().insert(getStr(), 1); // OK
    }();


QSet<int> getSet() { return {}; }

void testIntersect()
{
    QSet<int> other;
    auto s = getSet().intersect(other); // OK
}
