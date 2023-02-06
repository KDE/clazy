#include <QtCore/QObject>
#include <QtCore/QMetaMethod>

class TestObject : public QObject
{
    Q_OBJECT
public:
    TestObject(QObject *parent = nullptr) : QObject(parent) {}

public slots:
    void testSlot() {}
};

class TestCase : public QObject
{
    Q_OBJECT
private slots:
    void testQMetaMethod()
    {
        TestObject obj1;
        TestObject obj2;
        QMetaMethod signal = obj1.metaObject()->method(obj1.metaObject()->indexOfSignal("destroyed(QObject*)"));
        QMetaMethod slot = obj2.metaObject()->method(obj2.metaObject()->indexOfMethod("testSlot()"));
        QObject::connect(&obj1, signal, &obj2, slot);
    }
};
