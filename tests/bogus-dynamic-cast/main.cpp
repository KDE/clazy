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
}

class MyObj : public QObject
{
public:
};

void testQObjectCast(QObject *o)
{
    dynamic_cast<MyObj*>(o); // Warn
}

