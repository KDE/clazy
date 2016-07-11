#include <QtCore/QString>
#include <QtCore/QEvent>
#include <QtCore/QCoreApplication>

void postEvent()
{
    QEvent ev1(QEvent::None);
    QEvent *ev2 = new QEvent(QEvent::None);
    QObject *o;
    QCoreApplication::instance()->sendEvent(o, &ev1); // OK
    QCoreApplication::instance()->sendEvent(o, ev2); // Warning
    qApp->sendEvent(o, &ev1); // OK
    qApp->sendEvent(o, ev2); // Warning

    QCoreApplication::instance()->postEvent(o, &ev1); // Warning
    QCoreApplication::instance()->postEvent(o, ev2); // OK
    qApp->postEvent(o, &ev1); // Warning
    qApp->postEvent(o, ev2); // OK

    qApp->postEvent(o, new QEvent(QEvent::None)); // OK
}
