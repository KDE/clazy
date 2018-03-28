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
    virtual void myVirtualSig();
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

class MyDerivedObj : public MyObj
{
    Q_OBJECT
public:
    void myVirtualSig() override;
};

void test3()
{
    auto *o = new MyDerivedObj();
    QObject::connect(o, &MyDerivedObj::myVirtualSig, o, &MyObj::foo); // Warn, overridden but not a signal now
    QObject::connect(o, &MyObj::myVirtualSig, o, &MyObj::foo); // OK
}
