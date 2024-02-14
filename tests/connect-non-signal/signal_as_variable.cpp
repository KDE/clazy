#include <QtCore/QObject>
#include <QtCore/QTimer>




int main()
{
    auto *o = new QObject();
    auto d = &QObject::destroyed;
    QObject::connect(o, d, []() {}); // OK

    auto p = &QObject::property;
    QObject::connect(o, p, []() {}); // WARN, we use a getter here

    auto overload = qOverload<>(&QTimer::start); // Getting the method decl does not work in Qt5 for some reason
    QObject::connect(new QTimer, overload, []() {}); // WARN

    auto overload_static_cast = static_cast<void (QTimer::*)()>(&QTimer::start);
    QObject::connect(new QTimer, overload_static_cast, []() {}); // WARN

    auto overload_ok = qOverload<QObject*>(&QObject::destroyed);
    QObject::connect(o, overload_ok, []() {}); // WARN
}
