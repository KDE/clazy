#include <QtCore/QObject>

class Foo : public QObject
{
    Q_OBJECT
public:

    void foo()
    {
        emit scriptableSignal();
    }

Q_SIGNALS:
    Q_SCRIPTABLE void scriptableSignal(); // OK
};
