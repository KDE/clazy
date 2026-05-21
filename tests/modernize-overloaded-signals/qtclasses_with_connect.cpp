#include <QtCore/QTimer>
#include <QtWidgets/QMenu>

void dummyFnc()
{
}

class ReceiverClass : public QObject
{
    Q_OBJECT

public Q_SLOTS:
    void receiverSlot();
};

void qtimerConnect()
{
    auto obj = new ReceiverClass();
    QTimer::singleShot(0, &dummyFnc); // OK
    QTimer::singleShot(0, obj, &ReceiverClass::receiverSlot); // OK
    QTimer::singleShot(0, QOverload<>::of(&dummyFnc)); // OKish??? not a Method decl so we do not check
    QTimer::singleShot(0, obj, QOverload<>::of(&ReceiverClass::receiverSlot)); // WARN
}

void qMenuTest()
{
    auto obj = new ReceiverClass();
    QMenu menu;
    //menu.addAction("foo", obj, &ReceiverClass::receiverSlot); // OK
    menu.addAction("foo", obj, qOverload<>(&ReceiverClass::receiverSlot)); // WARN
    menu.addAction("foo", qOverload<>(&dummyFnc)); // OKish, make sure we can work with different counts
}
