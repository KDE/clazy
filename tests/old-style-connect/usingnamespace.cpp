#include <QtCore/QObject>

namespace Foo {
class MyObj : public QObject
{
    Q_OBJECT
public:

public Q_SLOTS:
    void slot1();
    void slot2();
Q_SIGNALS:
    void signal1();
};

}

using namespace Foo;

void foo()
{
    Foo::MyObj *o1 = new Foo::MyObj();
    QObject::connect(o1, SIGNAL(signal1()), o1, SLOT(slot1())); // Warning
}




