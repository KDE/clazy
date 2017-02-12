#include <QtCore/QObject>

class Foo : public QObject
{
    Q_OBJECT
public:
    Foo()
    {
        connect(this, static_cast<void (Foo::*)(int)>(&Foo::bar), [](int) {}); // OK
    }

Q_SIGNALS:
    void bar(int);
    void bar(float);
};
