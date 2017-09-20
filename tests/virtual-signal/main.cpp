#include <QtCore/QObject>

class MyObj : public QObject
{
    Q_OBJECT
public:
    void foo();
signals:
    void mySig(); // OK
    virtual void myVirtualSig(); // Warn
public Q_SLOTS:
    void mySlot();
};

