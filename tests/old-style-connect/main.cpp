#include <QtCore/QObject>
#include <QtCore/QTimer>
#include <QtCore/QState>
class MyObj : public QObject
{
    Q_OBJECT
public:

public Q_SLOTS:
    void slot1();
    void slot2();
    void slotWithArg(int i = 0);
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
    // QTimer doesn't support it with new connect syntax: Needs lambda
    QTimer::singleShot(0, o1, SLOT(slotWithArg()));
};


void MyObj::slot1()
{
    MyObj *o1;
    connect(o1, SIGNAL(signal1()), SLOT(slot2()));
}

class MyObjDerived : public MyObj
{
    Q_OBJECT
public:
    MyObjDerived()
    {
        connect(this, SIGNAL(signal1()), this, SLOT(slot2()));
        connect(this, SIGNAL(signal1()), SLOT(slot2()));
    }
};

void testDerived()
{
    MyObjDerived *o1;
    o1->connect(o1, SIGNAL(signal1()), o1, SLOT(slot2()));
    o1->connect(o1, SIGNAL(signal1()), SLOT(slot2()));
    QObject::connect(o1, SIGNAL(signal1()), o1, SLOT(slot2()));
}

class OtherObj : public QObject
{
    Q_OBJECT
public:
public Q_SLOTS:
    void otherSlot();
};

void testOther()
{
    OtherObj *other;
    MyObj *o1;
    other->connect(o1, SIGNAL(signal1()), SLOT(otherSlot()));
}

class WithNesting : public QObject
{
    Q_OBJECT
public Q_SLOTS:
    void slot1();
signals: void signal1();
private Q_SLOTS: void privSlot();
public:
    class Private;
    friend class Private;
};

class WithNesting::Private : public QObject
{
    Q_OBJECT
public:
    Private(WithNesting *q)
    {
        q->connect(q, SIGNAL(signal1()), SLOT(slot1()));
        connect(q, SIGNAL(signal1()), SLOT(pSlot1()));
        connect(q, SIGNAL(signal1()), q, SLOT(privSlot()));
        QTimer::singleShot(0, this, SLOT(privateSlot1())); // Testing if private slot gets fixed, it should due to "this"
    }
public Q_SLOTS:
    void pSlot1();
private Q_SLOT:
    void privateSlot1();
signals:
    void signal1();
};

void testNested()
{
     WithNesting::Private *p;
     QObject::connect(p, SIGNAL(signal1()), p, SLOT(privateSlot1())); // Warning, but can't fix, can't take address of private

     // QObject::connect(p, &WithNesting::Private::signal1, p, &WithNesting::Private::privateSlot1);
}

void testCharVariables(const char *methodName)
{
    QObject *o;
    QTimer::singleShot(0, o, methodName); // OK
    QObject::connect(o, methodName, 0, methodName); // OK
    o->connect(o, methodName, 0, methodName); // OK
    o->connect(o, SIGNAL(destroyed()), 0, methodName); // OK
}

void testQState()
{
    QState *s;
    QObject *o;
    s->addTransition(o, SIGNAL(destroyed()), s); // Warning
    s->addTransition(o, &QObject::destroyed, s); // OK

    WithNesting *wn;
    s->addTransition(wn, SLOT(privSlot()), s); // Warning, but can't fix
}

class TestingArgumentCounts : public QObject
{
    Q_OBJECT
public:
    TestingArgumentCounts()
    {
        connect(this, SIGNAL(si0()), SLOT(sl0())); // Warning and fixit
        connect(this, SIGNAL(si0()), SLOT(sl1(QString))); // Warning and can't fix
        connect(this, SIGNAL(si1(QString)), SLOT(sl0())); // Warning and can fix
        connect(this, SIGNAL(si2(QString)), SLOT(sl2())); // Warning and can't fix
    }
public Q_SLOTS:
    void sl0();
    void sl1(QString);
    void sl2(QString,QString,QString = QString());

signals:
    void si0();
    void si1(QString);
    void si2(QString,QString);
    void si3(QString,QString,QString);
};
