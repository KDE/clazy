#include <QtCore/QObject>
#include <QtCore/QDebug>
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
        QObject::connect(&o2, &QObject::destroyed, [&m]() { m; }); // OK, o2 is on the stack

        QObject *o3;
        QObject::connect(o3, &QObject::destroyed,
                         o3, [&o3] { o3; }); // OK, the captured variable is on the 3rd parameter too. It will get destroyed

        QObject::connect(o3, &QObject::destroyed, &o2, [&m]() { m; }); // OK, o2 is on the stack
    }

    int m;
};


void bug3rdArgument(QObject *sender)
{
    // Checks that we shouldn't warn when the captured variable matches the 3rd argument

    QObject context;
    QObject *obj;
    int m = 0;
    QObject::connect(sender, &QObject::destroyed, obj, [&] {
        qDebug() << m; // Warn
    });
    QObject::connect(sender, &QObject::destroyed, &context, [&] {
        qDebug() << context.objectName(); // OK
    });
}
