#include <QtCore/QObject>

void globalFunc();

void test()
{
    QObject *o;
    QObject::connect(o, &QObject::destroyed, o, []{}, Qt::UniqueConnection); // Warn
    QObject::connect(o, &QObject::destroyed, o, []{}, Qt::ConnectionType(Qt::UniqueConnection | Qt::DirectConnection)); // Warn
    QObject::connect(o, &QObject::destroyed, o, []{}); // OK
    QObject::connect(o, &QObject::destroyed, o, globalFunc); // OK
    QObject::connect(o, &QObject::destroyed, o, []{}, Qt::UniqueConnection); // Warn
    QObject::connect(o, &QObject::destroyed, o, globalFunc, Qt::UniqueConnection); // Warn
    QObject::connect(o, &QObject::destroyed, o, &QObject::deleteLater, Qt::UniqueConnection); // OK
    QObject::connect(o, QOverload<QObject*>::of(&QObject::destroyed), o, &QObject::deleteLater, Qt::UniqueConnection); // OK
    QObject::connect(o, &QObject::destroyed, o, QOverload<>::of(&QObject::deleteLater), Qt::UniqueConnection); // OK
}

