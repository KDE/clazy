#include <QtCore/QObject>















struct A
{
    virtual void foo() { }
};

struct B : public A
{

};

void test()
{
    A *a = new A();
    B *b = new B();

    dynamic_cast<A*>(b); // warning: Casting to base
    dynamic_cast<A*>(a); // warning: Casting to itself
    dynamic_cast<B*>(a); // OK
    dynamic_cast<B*>(b); // warning: Casting to itself
    static_cast<A*>(b); // warning: Casting to base
    static_cast<A*>(a); // warning: Casting to itself
    static_cast<B*>(a); // OK
    static_cast<B*>(b); // warning: Casting to itself
}

class MyObj : public QObject
{
    Q_OBJECT
public:
};

void testQObjectCast(QObject *o)
{
    dynamic_cast<MyObj*>(o); // Warn
    qobject_cast<QObject*>(o); // Warn
    MyObj myobj;
    qobject_cast<QObject*>(&myobj); // Warn
    qobject_cast<MyObj*>(o); // OK
}

class FwdDeclared;

class MyObj2 : public QObject
{
public:
    Q_DECLARE_PUBLIC(QObject) // OK, in macro
    MyObj2 *q_ptr;
};

struct Base2 {};
class Base1 : public QObject {};

class Derived : public Base1, public Base2
{
public:
    void test()
    {
        static_cast<Base1*>(this); // OK
        static_cast<Base2*>(this); // OK
    }
};

void test2()
{
    static_cast<MyObj*>(nullptr); // OK
    static_cast<MyObj*>(0); // OK

    MyObj *o1;
    MyObj2 *o2;
    true ? static_cast<QObject*>(o1) : static_cast<QObject*>(o2); // Ok
    true ? static_cast<QObject*>(o1) : static_cast<QObject*>(o2); // Ok
}
class MyObj4 : public QObject
{
    Q_OBJECT
public:
    void test()
    {
        qobject_cast<MyObj4*>(sender()); // OK
    }
};
