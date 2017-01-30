#include <QtCore/QObject>

class Foo: public QObject
{
    Q_OBJECT
public:
    Foo()
    {
        connect(this, &Foo::bar, []{});
    }
Q_SIGNALS:
    void bar();
};
