#include <QtCore/QObject>

struct A
{
    int v;
};


void test()
{
    QObject *o;
    int a, b, c;
    auto f = [&a]() {}; // OK
    o->connect(o, &QObject::destroyed, [a]() {}); // OK
    o->connect(o, &QObject::destroyed, [&a]() {}); // Warning
    QObject::connect(o, &QObject::destroyed, [&a]() { }); // Warning
    QObject::connect(o, &QObject::destroyed, [&]() { a; b; }); // Warning
    QObject::connect(o, &QObject::destroyed, [=]() { a; b; }); // OK

    A *a1;
    QObject::connect(o, &QObject::destroyed, [a1]() { a1->v; }); // OK
    QObject::connect(o, &QObject::destroyed, [&a1]() { a1->v; }); // Warning
}


static int s;

struct C
{

    void foo()
    {
        QObject *o;
        int m;
        QObject::connect(o, &QObject::destroyed, [this]() { }); // OK
        QObject::connect(o, &QObject::destroyed, []() { s; }); // OK
        QObject::connect(o, &QObject::destroyed, [&m]() { m; }); // Warn

        QObject o2;
        QObject::connect(&o2, &QObject::destroyed, [&m]() { m; }); // OK, o is on the stack

        QObject *o3;
        QObject::connect(o3, &QObject::destroyed,
                         o3, [&o3] { o3; }); // OK, the captured variable is on the 3rd parameter too. It will get destroyed
    }

    int m;
};
