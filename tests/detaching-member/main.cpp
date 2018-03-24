#include <QtCore/QList>
#include <QtCore/QString>
#include <QtCore/QMap>
#include <QtCore/QVector>










struct A
{
    A()
    {
        m_list.begin();  // Warning
        m_list.cbegin(); // OK
    }

    void f()
    {
        QList<int> list;
        list.begin(); // OK
        list[0]; // OK

        m_list.begin();  // Warning
        m_list.cbegin(); // OK
        m_constList.begin();  // OK
    }

    void constF() const
    {
        m_list.begin(); // OK
        m_string.data(); // OK
        m_string[0]; // OK
        m_list[0]; // OK
        m_mutableString.begin(); // Warning
        m_mutableString[0]; // Warning
    }

    void testAssignment()
    {
        m_string[0] = QChar('a'); // OK
        m_mutableString[0] = QChar('a'); // OK
        m_list[0]++;    // OK
        m_list[0] += 2; // OK
        m_list2[0] = m_list[0]; // Warning
    }


    QList<int> m_list;
    QList<int> m_list2;
    QString m_string;
    mutable QString m_mutableString;
    const QList<int> m_constList;
};

class Static
{
public:
    static Static *instance()
    {
        return new Static();
    }

    QMap<int,int> getMap() const { return {}; }
    QMap<QString,QString> map;
};

void test(const QString &prefix, const QString &path)
{
    Static::instance()->getMap()[0] = 0;
    Static::instance()->getMap()[0] += 0;
    Static::instance()->map[prefix] = path;
    Static::instance()->map[prefix] += path;
}

// Bug 356699
struct T {
    void nonConstMethod() {}
    void constMethod() const { }
};

struct S
{
    QList<T> m_listOfValues;
    QList<T*> m_listOfPointers;
};

void test356699()
{
    S s;
    s.m_listOfPointers[0]->nonConstMethod(); // Warning
    s.m_listOfValues[0].nonConstMethod(); // OK
    s.m_listOfValues.at(0).constMethod(); // OK
    s.m_listOfPointers.at(0)->nonConstMethod(); // OK
}

void test356755()
{
    S s;
    qSort(s.m_listOfPointers.begin(), s.m_listOfPointers.end());
}

void testFill() {
    struct Fill {
        Fill() {
            m_vector.fill(1);
        }
        QVector<int> m_vector;
    };

}
