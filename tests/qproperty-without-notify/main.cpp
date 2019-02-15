#include <QtCore/QObject>

class MyObj : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int foo READ foo) // Warn
    Q_PROPERTY(int foo1 READ foo1 CONSTANT) // OK
    Q_PROPERTY(int foo2 READ foo2 NOTIFY fooChanged) // KO
    Q_PROPERTY(int foo3 READ foo3 WRITE setFoo) // Warn
};

class MyGadget
{
    Q_GADGET
    Q_PROPERTY(int foo READ foo) // Ok
};

class MyObj2 : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int foo READ foo) // Warn
    Q_PROPERTY(int foo1 READ foo1) // Warn
    Q_PROPERTY(int foo2 READ foo2
        NOTIFY foo2Changed) // Multiline case. OK
    Q_PROPERTY(int foo3 READ foo3 NOTIFY
        foo3Changed) // Another multiline case. OK
};
