#include <QtCore/QObject>

class MyObj : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int foo READ foo) // Warn
    Q_PROPERTY(int foo1 READ foo1 CONSTANT) // OK
    Q_PROPERTY(int foo2 READ foo2 NOTIFY fooChanged) // KO
    Q_PROPERTY(int foo3 READ foo3 WRITE setFoo) // Warn
};

