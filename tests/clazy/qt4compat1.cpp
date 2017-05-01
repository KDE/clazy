#include <QtCore/QObject>

void test()
{
    QObject o;
    o.connect(&o, SIGNAL(foo()), SLOT(bar()));
}
