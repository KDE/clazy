#include <QtCore/QObject>

namespace Foo2 {
class MyObj : public QObject
{
    Q_OBJECT
public:

public Q_SLOTS:
    void slot1() {}
    void slot2() {}
Q_SIGNALS:
    void signal1();
};

}

using namespace Foo2;

void foo1()
{
    Foo2::MyObj *o1 = new Foo2::MyObj();
    QObject::connect(o1, SIGNAL(signal1()), o1, SLOT(slot1())); // Warning
}

int main() { return 0; }

#include "usingnamespace.moc"
