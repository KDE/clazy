#include <QtCore/QVector>

// this test only passes on Qt >= 5.3
void test()
{
    QVector<int> a,b,c,d,e;
    foreach (int i2, d)
        e << 1; // Warning
}
