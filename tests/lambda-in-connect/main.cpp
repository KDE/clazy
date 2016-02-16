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
        QObject::connect(o, &QObject::destroyed, [this]() { }); // OK
        QObject::connect(o, &QObject::destroyed, []() { s; }); // OK
    }

    int m;
};
