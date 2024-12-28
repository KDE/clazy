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

#if QT_VERSION_MAJOR == 5
#include "usingnamespace.qt5.moc_"
#elif Q_MOC_OUTPUT_REVISION == 69
#include "usingnamespace.qt6.moc_69"
#else
#include "usingnamespace.qt6.moc_"
#endif
