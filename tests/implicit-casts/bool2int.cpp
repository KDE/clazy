#include <QtCore/QDebug>

struct S {
    void operator=(int);
};

void foo(int) {}

extern "C" void takesInt_C(int);
extern void takesInt_Cpp(int);

struct A
{
    void foo(int);
};

void testIntToBool(bool b1)
{
    bool b2;
    foo(false); // Warn
    foo(b1); // Warn
    foo(b2); // Warn
    A a;
    a.foo(false); // Warn

    takesInt_C(false); // OK
    takesInt_Cpp(false); // Warn

    S s; s = false; // OK
    qDebug("Foo %d", false); // OK
}
