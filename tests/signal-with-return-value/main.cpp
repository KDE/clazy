#include <QtCore/QObject>
#include <QtCore/QString>

class Foo : public QObject
{
Q_OBJECT
signals:
    void voidSig(); // ok
    int intSig(); // Warn
    void* voidStarSig(); // warn
    Q_SCRIPTABLE int scriptableIntSig(); // ok
};

