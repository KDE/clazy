#include <QtCore/QObject>

class MyObj : public QObject
{
    Q_OBJECT
public:
    MyObj();
    void mySlot(int);
signals:
    void mySig();
    void mySig(int);
    void mySig2(int) const;
};

void test()
{
    MyObj *o;
    o->connect(o, qOverload<int>(&MyObj::mySig), o, &MyObj::mySlot); // OK
    o->connect(o, qConstOverload<int>(&MyObj::mySig2), o, &MyObj::mySlot); // OK
    o->connect(o, qNonConstOverload<int>(&MyObj::mySig), o, &MyObj::mySlot); // OK
    o->connect(o, qOverload<int>(&MyObj::mySlot), o, &MyObj::mySlot); // Warn
}
