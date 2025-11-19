#include <QtCore/QObject>
#include <QtCore/QScopedPointer>
#include <QtCore/QPointer>
#include <QtCore/QSharedPointer>
class MyObj : public QObject {public: void doStuff();};
void test()
{
    QScopedPointer<QObject> ptr(new QObject);
    ptr.data()->deleteLater(); // Warn

    QPointer<QObject> qptr(new QObject);
    qptr.data()->deleteLater();   // Warn

    QSharedPointer<QObject> qsptr(new QObject);
    qsptr.  data()->deleteLater(); // Warn

    QScopedPointer<QObject> okPtr(new QObject);
    okPtr->deleteLater();

    static_cast<MyObj*>(qsptr.data())->doStuff();

    QPointer<MyObj> obj(new MyObj());
    QObject::connect(obj, &QObject::objectNameChanged, obj, &MyObj::doStuff);
    QObject::connect(obj, &QObject::objectNameChanged, obj.data(), &MyObj::doStuff); // WARN
    QObject::connect(obj.data(), &QObject::objectNameChanged, obj, &MyObj::doStuff); // WARN
    QObject::connect(ptr.data(), &QObject::objectNameChanged, obj.data(), &MyObj::doStuff); // OK, no overload for QScopedPointer
}

