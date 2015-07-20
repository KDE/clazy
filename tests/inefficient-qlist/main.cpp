#include <QtCore/QList>










struct SmallType {
    char a[8];
};


struct BigType {
    char a[9];
};


void foo()
{
    QList<BigType> bigT;
    QList<SmallType> smallT;
}

class A {
public:
    void foo()
    {
        m_big.clear();
    }

    QList<BigType> m_big;
};

void foo1(QList<BigType> big2)
{
    QList<BigType> bigt;
    bigt = big2;
}



