#include <QtCore/QObject>

namespace Foo
{
    class MyObj2 : public QObject
    {
        Q_OBJECT
    public Q_SLOTS:
        void separateNSSlot();
    };
}

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


void foo()
{
    Foo::MyObj *o1 = new Foo::MyObj();
    MyObj2 *o2;
    QObject::connect(o1, SIGNAL(signal1()), o1, SLOT(slot1())); // Warning
    QObject::connect(o1, SIGNAL(signal1()), o2, SLOT(separateNSSlot())); // Warning
}


}
