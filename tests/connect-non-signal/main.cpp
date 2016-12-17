#include <QtCore/QObject>

class MyObj;
class MyObj : public QObject
{
    Q_OBJECT
public:
    void foo();
    int bar() { return 1; }
signals:
    void mySig();
public Q_SLOTS:
    void mySlot();
};

void test()
{
    auto *o = new MyObj();
    o->connect(o, &MyObj::mySig, o, &MyObj::mySlot); // OK
    o->connect(o, &MyObj::foo, o, &MyObj::mySlot); // Warning
    o->connect(o, SIGNAL(foo()), o, SLOT(mySlot())); // OK, only PMF is checked
}

class MyObj2 : public QObject
{
    Q_OBJECT
public:
    void foo();
signals:
    void mySig();
};

void test2()
{
    auto *o2 = new MyObj2();
    o2->connect(o2, &MyObj2::mySig, o2, &MyObj2::foo); // OK
    QObject::connect(o2, &MyObj2::mySig, o2, &MyObj2::foo); // OK
    QObject::connect(o2, &MyObj2::foo, o2, &MyObj2::foo); // OK
}
