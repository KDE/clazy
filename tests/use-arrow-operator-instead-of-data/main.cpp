#include <QtCore/QObject>
#include <QtCore/QScopedPointer>
#include <QtCore/QPointer>
#include <QtCore/QSharedPointer>

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
}

