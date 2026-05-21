#include <QtCore/QObject>
#include <QtCore/QString>

class Base : public QObject
{
    Q_OBJECT

Q_SIGNALS:
    void mySignal();
};

class Subclass : public Base
{
    Q_OBJECT
Q_SIGNALS:
    void mySignal(const QString &str);
};

void testConnect()
{
    auto obj = new Subclass();
    QObject::connect(obj, qOverload<const QString &>(&Subclass::mySignal), []() { }); // WARN, overload unneeded
    QObject::connect(obj, qOverload<>(&Base::mySignal), []() { }); // WARN, overload unneeded
}
