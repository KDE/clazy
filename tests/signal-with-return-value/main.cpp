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

    void withArgs1(const Foo f); // OK
    void withArgs2(const Foo &f); // OK
    void withArgs3(Foo f); // OK
    void withArgs4(Foo &f); // Warn
};

