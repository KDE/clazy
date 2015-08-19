#include <QtCore/qmetatype.h>
#include <QtCore/qhash.h>
#include <QtCore/QTextStream>



const char *cstring = "foo";
void foo(int) {}
bool someBool() { return true; }
void foo1()
{
    bool b;
    foo(b);
    foo(someBool());
    foo('a');
    enum  A { Defined = true };
    bool b2;
    if (b == b2) return;
    if (b) return;
    foo((int)b);
}

void foo2(const bool) {}

void test()
{
    QHash<QString, bool> hash123;
    bool b;
    const bool b123 = b; // OK
    foo2(b); // OK
    hash123.insert("foo", b); // OK

    QTextStream stream;
    stream << b;

    QAtomicInt ref;
    ref = b;
}

struct A
{
    A() : f(false), f2(false) {}
    QAtomicInt f;
    uint f2;
    void a()
    {
        bool b;
        foo(qint8(b));
    }
};