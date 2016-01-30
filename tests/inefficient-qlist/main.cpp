#include <QtCore/QList>










struct SmallType {
    char a[8];
};


struct BigType {
    char a[9];
};


void foo()
{
    QList<BigType> bigT; // Warning
    QList<SmallType> smallT; // OK
}

class A {
public:
    void foo()
    {
        m_big.clear();
    }

    QList<BigType> m_big; // Warning
};

void foo1(QList<BigType> big2)
{
    QList<BigType> bigt; // Warning
    bigt = big2;
}


void test_bug358740()
{
    QList<int> list; // OK
    int a, b;
}
