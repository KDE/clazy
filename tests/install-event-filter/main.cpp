#include <QtCore/QObject>

class MyObject_hasEventFilter : public QObject
{
protected:
    bool eventFilter(QObject *, QEvent *) override;
};

class MyObject_doesntHaveEventFilter : public QObject
{
};

class Obj : public QObject
{
public:
    Obj(MyObject_hasEventFilter* other)
    {
        other->installEventFilter(this);  // OK
        other->installEventFilter(other); // OK
        installEventFilter(other); // OK
        installEventFilter(this); // Warning
        this->installEventFilter(other); // OK
    }

    void test2(MyObject_doesntHaveEventFilter *other)
    {
        other->installEventFilter(this); // OK
        other->installEventFilter(other); // OK
        installEventFilter(this); // Warning
        this->installEventFilter(other);  // Warning
        installEventFilter(other); // Warning
    }
};

class DerivedDerived : public Obj {};
class DerivedDerived2 : public MyObject_hasEventFilter {};

class TestBiggerHierarchy : public QObject
{
    void test(DerivedDerived *d1, DerivedDerived2 *d2)
    {
        installEventFilter(d1); // Warning
        installEventFilter(d2); // OK
    }
};
