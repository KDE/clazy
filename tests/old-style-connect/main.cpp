#include <QtCore/QObject>
#include <QtCore/QTimer>

class MyObj : public QObject
{
    Q_OBJECT
public:

public Q_SLOTS:
    void slot1();
    void slot2();
Q_SIGNALS:
    void signal1();
};

void foo()
{
    MyObj *o1;
    MyObj *o2;

    o1->connect(o1, SIGNAL(signal1()), o2, SLOT(slot1()));
    o1->connect(o1, SIGNAL(signal1()), SLOT(slot1()));
    o1->connect(o1, SIGNAL(signal1()), SIGNAL(signal1()));
    QObject::connect(o1, SIGNAL(signal1()), o2, SIGNAL(signal1()));
    QObject::disconnect(o1, SIGNAL(signal1()), o2, SIGNAL(signal1()));

    o1->connect(o1, &MyObj::signal1, &MyObj::signal1);
    QObject::connect(o1, &MyObj::signal1, o2, &MyObj::signal1);
    QObject::disconnect(o1, &MyObj::signal1, o2, &MyObj::signal1);

    QTimer::singleShot(0, o1, SLOT(slot1()));
    QTimer::singleShot(0, o1, &MyObj::slot1);
};
