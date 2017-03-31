#include <QtCore/QString>
#include <QtCore/QList>

struct BigTrivial
{
    int a, b, c, d, e;
};

void test()
{
    QList<BigTrivial> list;
    foreach (BigTrivial b, list) { }
}
