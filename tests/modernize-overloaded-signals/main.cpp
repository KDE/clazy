#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QCoreApplication>

class MyClass : public QObject
{
    Q_OBJECT

Q_SIGNALS:
    void noOverloadNeeded();
    void noOverloadNeededWithArgs(const QString &bla);

    void overloadedArgs(const QString &);
    void overloadedArgs();
};

void testSignalParameter()
{
    auto obj = new MyClass();
    // Overload unneeded
    QObject::connect(obj, &MyClass::noOverloadNeeded, qApp, [](){}); // OK
    QObject::connect(obj, qOverload<>(&MyClass::noOverloadNeeded), qApp, [](){}); // WARN
    QObject::connect(obj, QOverload<>::of(&MyClass::noOverloadNeeded), qApp, [](){}); // WARN
    QObject::connect(obj, static_cast<void (MyClass::*)()>(&MyClass::noOverloadNeeded), qApp, [](){}); // WARN

    // Overload unneeded, but signal has args
    QObject::connect(obj, &MyClass::noOverloadNeededWithArgs, qApp, [](){}); // OK
    QObject::connect(obj, qOverload<const QString &>(&MyClass::noOverloadNeededWithArgs), qApp, [](){}); // WARN
    QObject::connect(obj, qOverload<const QString &>(&MyClass::noOverloadNeededWithArgs), qApp, [](){}); // WARN
    QObject::connect(obj, QOverload<const QString &>::of(&MyClass::noOverloadNeededWithArgs), qApp, [](){}); // WARN
    QObject::connect(obj, static_cast<void (MyClass::*)(const QString &)>(&MyClass::noOverloadNeededWithArgs), qApp, [](){}); // WARN

    // Overload neeeded, but syntax could be improved
    QObject::connect(obj, QOverload<const QString &>::of(&MyClass::overloadedArgs), qApp, [](){}); //WARN
    QObject::connect(obj, static_cast<void (MyClass::*)(const QString &)>(&MyClass::overloadedArgs), qApp, [](){}); // WARN
}
