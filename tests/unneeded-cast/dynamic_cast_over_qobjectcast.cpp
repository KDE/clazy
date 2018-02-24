#include <QtCore/QObject>

struct MyObj : public QObject {};

void testQObjectCast(QObject *o)
{
    dynamic_cast<MyObj*>(o); // OK
}
