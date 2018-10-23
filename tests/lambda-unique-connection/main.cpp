#include <QtCore/QObject>

void globalFunc();

void test()
{
    QObject *o;
    o->connect(o, &QObject::destroyed, o, []{}, Qt::UniqueConnection); // Warn
    o->connect(o, &QObject::destroyed, o, []{}, Qt::ConnectionType(Qt::UniqueConnection | Qt::DirectConnection)); // Warn
    o->connect(o, &QObject::destroyed, o, []{}); // OK
    o->connect(o, &QObject::destroyed, o, globalFunc); // OK
    QObject::connect(o, &QObject::destroyed, o, []{}, Qt::UniqueConnection); // Warn
    o->connect(o, &QObject::destroyed, o, globalFunc, Qt::UniqueConnection); // Warn
    o->connect(o, &QObject::destroyed, o, &QObject::deleteLater, Qt::UniqueConnection); // OK
    o->connect(o, QOverload<QObject*>::of(&QObject::destroyed), o, &QObject::deleteLater, Qt::UniqueConnection); // OK
    o->connect(o, &QObject::destroyed, o, QOverload<>::of(&QObject::deleteLater), Qt::UniqueConnection); // OK
}

