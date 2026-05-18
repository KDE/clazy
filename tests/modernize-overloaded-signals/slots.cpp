#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QCoreApplication>


class MyClass : public QObject
{
    Q_OBJECT

Q_SIGNALS:
    void dummySignal(const QString &str);

public Q_SLOTS:
    void noOverloadedSlot();
    void noOverloadedSlotWithArgs(const QString &str);
    void overloadedSlot(const QString &);
    void overloadedSlot();
};

void testSlotParameter()
{
    auto obj = new MyClass();
    QObject::connect(obj, &MyClass::dummySignal, obj, &MyClass::noOverloadedSlot); // OK
    QObject::connect(obj, &MyClass::dummySignal, obj, &MyClass::noOverloadedSlotWithArgs); // OK
    QObject::connect(obj, &MyClass::dummySignal, obj, qOverload<const QString&>(&MyClass::overloadedSlot)); // OK
    // Overload unneeded
    QObject::connect(obj, &MyClass::dummySignal, obj, qOverload<>(&MyClass::noOverloadedSlot)); // WARN
    QObject::connect(obj, &MyClass::dummySignal, obj, QOverload<>::of(&MyClass::noOverloadedSlot)); // WARN
    QObject::connect(obj, &MyClass::dummySignal, obj, static_cast<void (MyClass::*)()>(&MyClass::noOverloadedSlot)); // WARN
    QObject::connect(obj, &MyClass::dummySignal, obj, qOverload<const QString &>(&MyClass::noOverloadedSlotWithArgs)); // WARN
    QObject::connect(obj, &MyClass::dummySignal, obj, QOverload<const QString &>::of(&MyClass::noOverloadedSlotWithArgs)); // WARN
    QObject::connect(obj, &MyClass::dummySignal, obj, static_cast<void (MyClass::*)(const QString &)>(&MyClass::noOverloadedSlotWithArgs)); // WARN
  
    // Can be simplified
    QObject::connect(obj, &MyClass::dummySignal, obj, QOverload<>::of(&MyClass::overloadedSlot)); // WARN
    QObject::connect(obj, &MyClass::dummySignal, obj, QOverload<const QString &>::of(&MyClass::overloadedSlot)); // WARN
    QObject::connect(obj, &MyClass::dummySignal, obj, static_cast<void (MyClass::*)(const QString &)>(&MyClass::overloadedSlot)); // WARN

}
