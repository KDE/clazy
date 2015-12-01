#include <QtCore/qmetatype.h>
#include <QtCore/qhash.h>
#include <QtCore/QTextStream>
#include <QtTest/QTest>


const char *cstring = "foo";
void foo(int) {}
bool someBool() { return true; }
void foo1()
{
    bool b;
    foo(b); // Warning
    foo(someBool()); // Warning
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
    unsigned int f2;
    void a()
    {
        bool b;
        foo(qint8(b));
    }
};

void takesBool(bool, A*) {}
void setEnabled(bool) {}

struct B
{
    B(bool, A*)
    {
    }

};

void testPointerToInt()
{
    A *a;
    takesBool(a, a); // Warning
    setEnabled(a);
    B b(a, a); // Warning
    bool boo1;
}

void testQStringArgOK()
{
    bool b;
    QString().arg(b);
}


void testQVERIFY() // Bug 354064
{
    A *a;
    QVERIFY(a); // OK
    QVERIFY2(a, "msg"); // OK
}
