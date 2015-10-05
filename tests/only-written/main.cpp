#include <QtCore/qglobal.h>
#include <QtCore/QList>
#include <QtCore/QByteArray>
#include <QtCore/QString>
#include <QtCore/QVector>

void dummy_function(int a)
{

}

void test()
{
    int a = 0; // Warning
    a = 1;
}

void test2()
{
    int a = 0; // OK
    dummy_function(a);
}

void test3()
{
    int a = 0, b = 0; // Warning on b
    a = b;
}

int test4()
{
    int a = 0; // OK
    return a;
}

void test5()
{
    int a = 0; // Warning
    a++;
}

void test6()
{
    int a = 0; // OK
    if (a) {}
}

class A
{
public:
    A()
    {
        // Testing in CTOR
        int a = 0; // Warning
        a++;
    }

    A& operator=(const A&) { return *this; };
};

void test7()
{
    A a;
    a = A(); // OK
}

struct test8 {
    int m_value;
    void test()
    {
        m_value++;
    }
};

void test9(int a) // Warning
{
    a++;
}

void test10(int &a)
{
    a++; // OK
}


void accept_by_ptr(int *a) {};

void test11()
{
    int a = 0;
    accept_by_ptr(&a); // Ok
}


class MemberInitTest
{
public:
    MemberInitTest(int v) : m_value(v) {} // OK
    int m_value;
    int m_array[10];
};

int dummy_function2(bool b) { return 1; };

void test13() // OK
{
    int a = 0; // Warning
    a = 1;
    if (!a) {}
}

void test14(int &other)
{
    int a = 0; // OK
    other = a++;
}

void test15()
{
    MemberInitTest m(1);
    int a = 0;
    m.m_array[a++] = 1; // OK
}

void test16()
{
    int a = 0;
    if (a-- == 0) { // OK
    }
}

void test17()
{
    int a = 0;
    while (a-- == 0) { // OK
    }

    int b = 0;
    QVector<int> daynums(1, b);
}

template<class T, bool b>
bool test18()
{
    if (b) {} // OK
};

void test19(int a)
{
    (void)a; // OK
}

void test20()
{
    int a = 0;
    int b = true ? a : 0; // OK
}

void test21()
{
    for (int a= 5; a--;) { } // OK
}

struct A1 {
    int a;
};

void test22(int a)
{
    A1 a1 = {a};
}

void test23()
{
    QList<int> a;
    a.append(1); //  Warning
}

int test24()
{
    QList<int> a; // OK
    a.append(1);
    return a.count();
}

void test25()
{
    QList<int> a; // OK
    a.append(1);
    if (a.count()) {}
}

void test26()
{
    QList<int> a; // OK
    a.append(1);
    int i = a.takeAt(0);
}

void test27()
{
    QList<int> a;
    foreach (int i, a) {} // OK
}

void test28()
{
    QList<int> a;
    for (int i : a) {} // OK
}

void test29()
{
    QList<A1*> a;
    a.append(new A1());
    qDeleteAll(a);
}

typedef QList<A1*> A1List;
class WithStaticMethod
{
public:
    static void staticFoo(const A1List &a) {}
};

void test30()
{
    A1List a;
    a.append(new A1());
    WithStaticMethod::staticFoo(a);
}


template <int edge>
void compare()
{
    switch (edge)
    {
        default: break;
    }
}
