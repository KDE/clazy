#include <QtCore/QSet>

void test()
{
    QSet<int> s1, s2;
    s1.intersects(s2); // OK
    s1.intersect(s2).isEmpty(); // Warning
}
