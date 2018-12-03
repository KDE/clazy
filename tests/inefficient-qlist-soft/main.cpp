#include <QtCore/QList>










struct SmallType {
    char a[sizeof(void*)];
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

    QList<BigType> m_big; // OK
};

void foo1(QList<BigType> big2)
{
    QList<BigType> bigt; // OK
    bigt = big2;
}

QList<BigType> foo2()
{
    QList<BigType> bigt; // OK
    return bigt;
}

static QList<BigType> s_big;
extern void takesBig(QList<BigType>);
void foo3()
{
    QList<BigType> bigt; // OK
    s_big = bigt;
    s_big = QList<BigType>(); // OK

    QList<BigType> bigt2; // OK
    takesBig(bigt2);
}
extern QList<BigType> returnsBig();
void foo4()
{
    QList<BigType> b; // Warning
    for (auto a : b) {}

    QList<BigType> b2 = returnsBig(); // OK
    QList<BigType> b3 = { BigType() }; // Warning
    QList<BigType> b4 = b2; // Ok
}

struct A1
{
    QList<BigType> a();
};

struct B1
{
    A1 b();
};

void foo5()
{
    QList<BigType> b5 = B1().b().a();
}

void test_bug358740()
{
    QList<int> list; // OK
    int a, b;
}
