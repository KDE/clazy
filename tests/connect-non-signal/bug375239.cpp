#include <QtCore/QObject>

#define DECLARE_SIGNAL(name) \
    Q_SIGNALS: \
        void name();

class MyClass : public QObject
{
    Q_OBJECT

public:
    explicit MyClass(QObject *parent = 0);
    DECLARE_SIGNAL(mySignal)

public slots:
    void mySlot();
};

MyClass::MyClass(QObject *parent) : QObject(parent)
{
    connect(this, &MyClass::mySignal, this, &MyClass::mySlot);
}
